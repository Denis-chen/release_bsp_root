/*
 * $QNXLicenseC:
 * Copyright 2009, QNX Software Systems.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"). You
 * may not reproduce, modify or distribute this software except in
 * compliance with the License. You may obtain a copy of the License
 * at: http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTIES OF ANY KIND, either express or implied.
 *
 * This file may contain contributions from others, either as
 * contributors under the License or as licensors under other terms.
 * Please review this entire file for other proprietary rights or license
 * notices, as well as the QNX Development Suite License Guide at
 * http://licensing.qnx.com/license-guide/ for other information.
 * $
 */


#include "startup.h"
#include <arm/mx25.h>
#include "mx35_iomux.h"
#include "i2c.h"

static int tx_byte(unsigned char *data, unsigned int base);
static int rx_byte(unsigned char *data, unsigned int base);
static int wait_op_done(unsigned int base, int is_tx);
static inline int is_bus_free(unsigned int base);
static inline int wait_till_busy(unsigned int base);


// Mini I2C driver support on i.MX25 3stack platforms


void init_i2c_clock(void)
{
    // init I2C clock rate
    out16(I2C1_BASE_ADDR + I2C_IFDR, I2C_CLK_40000);
}

int i2c_xfer(unsigned char slave_addr, unsigned char reg, unsigned char* val, int dir)
{
    volatile int i;
    unsigned int base = I2C1_BASE_ADDR;
    unsigned char data;
    unsigned short i2cr;
    int ret = 0;

    // reset and enable I2C1
    out16(I2C1_BASE_ADDR + I2C_I2CR, 0);
    out16(I2C1_BASE_ADDR + I2C_I2CR, I2C_I2CR_IEN);

    // wait at least 2 cycles of per_clk, declare i as volatile to avoid optimization 
    for (i = 0; i < 100; i++) ;

    // Step 1: generate START signal
    // 1.1 make sure bus is free
    if (!is_bus_free(base)) {
        kprintf("Error %d: Bus is not free\n", __LINE__);
        return -1;
    }

    // 1.2 clear both IAL and IIF bits
    out16(base + I2C_I2SR, 0);

    // 1.3 assert START signal and also indicate TX mode
    i2cr = I2C_I2CR_IEN | I2C_I2CR_MSTA | I2C_I2CR_MTX;
    out16(base + I2C_I2CR, i2cr);

    // 1.4 make sure bus is busy after the START signal
    if (wait_till_busy(base) != 0) {
        kprintf("Error %d: Bus is not busy\n", __LINE__);
        return -1;
    }

    // Step 2: send slave address + read/write at the LSB
    data = ((slave_addr << 1) | I2C_WRITE) & 0xFF;
    if (tx_byte(&data, base) != 0) {
        kprintf("Error %d: Failed to transmit slave address\n", __LINE__);
        return -1;
    }

    // Step 3: send I2C device register address
    data = reg & 0xFF;
    if (tx_byte(&data, base) != 0) {
        kprintf("Error %d: Failed to transmit device register address\n", __LINE__);
        return -1;
    }

    // Step 4: read/write data
    if (dir == I2C_READ) {
        // do repeat-start
        i2cr = in16(base + I2C_I2CR);
        out16(base + I2C_I2CR, (i2cr | I2C_I2CR_RSTA));

        // send slave address again, but indicate read operation
        data = (slave_addr << 1) | I2C_READ;
        if (tx_byte(&data, base) != 0) {
            kprintf("Error %d: Failed to transmit slave address\n", __LINE__);
            return -1;
        }

        // change to receive mode
        i2cr = in16(base + I2C_I2CR);

        // read only one byte, make sure don't send ack
        i2cr |= I2C_I2CR_TXAK;
        i2cr &= ~I2C_I2CR_MTX;
        out16(base + I2C_I2CR, i2cr);

        // dummy read
        in16(base + I2C_I2DR);

        // now reading ...
        if (rx_byte(&data, base) != 0) {
            kprintf("Error %d: Failed to receive data\n", __LINE__);
            return -1;
        }
        *val = data;

    } else {    // I2C_WRITE
        data = *val & 0xFF;
        if((ret = tx_byte(&data, base)) != 0) {
            kprintf("Error %d: Failed to transmit data\n", __LINE__);
        }

        // generate STOP by clearing MSTA bit
        out16(base + I2C_I2CR, (I2C_I2CR_IEN | I2C_I2CR_MTX));
    }

    return ret;
}

// For master TX, always expect a RXAK signal to be set!
static int tx_byte(unsigned char *data, unsigned int base)
{
    // clear both IAL and IIF bits
    out16(base + I2C_I2SR, 0);

    // transmit the data
    out16(base + I2C_I2DR, *data);

    if (wait_op_done(base, 1) != 0)
        return -1;

    return 0;
}

// For master RX
static int rx_byte(unsigned char *data, unsigned int base)
{
    unsigned short i2cr;

    if (wait_op_done(base, 0) != 0)
        return -1;

    // clear both IAL and IIF bits
    out16(base + I2C_I2SR, 0);

    // last byte --> generate STOP
    i2cr = in16(base + I2C_I2CR);
    out16(base + I2C_I2CR, (i2cr & ~(I2C_I2CR_MSTA | I2C_I2CR_MTX)));

    *data = in16(base + I2C_I2DR);

    return 0;
}

static inline int is_bus_free(unsigned int base)
{
    return ((in16(base + I2C_I2SR) & I2C_I2SR_IBB) == 0);
}

static inline int wait_till_busy(unsigned int base)
{
    int i = I2C_WAIT_CNT;

    while (((in16(base + I2C_I2SR) & I2C_I2SR_IBB) == 0) && (--i > 0)) {
        if (in16(base + I2C_I2SR) & I2C_I2SR_IAL) {
            kprintf("Error %d: arbitration lost!\n", __LINE__);
           return -1;
        }
    }

    if (i <= 0) {
        kprintf("Error %d: timeout unexpected\n", __LINE__);
        return -1;
    }

    return 0;
}

static int wait_op_done(unsigned int base, int is_tx)
{
    volatile unsigned short v;
    int i = WAIT_RXAK_LOOPS;

    while ((((v = in16(base + I2C_I2SR)) & I2C_I2SR_IIF) == 0 ||
           (v & I2C_I2SR_ICF) == 0) && --i > 0) {

        if (v & I2C_I2SR_IAL) {
            kprintf("Error %d: Arbitration lost\n", __LINE__);
            return -1;
        }
    }

    if (i <= 0) {
        kprintf("Error %d: timeout unexpected\n", __LINE__);
        return -1;
    }
    if (is_tx) {
        if (v & I2C_I2SR_IAL) {
            kprintf("Error %d: Arbitration lost\n", __LINE__);
            return -1;
        }
        if (v & I2C_I2SR_RXAK) {
            kprintf("Error %d: no ack received\n", __LINE__);
            return -1;
        }
    }
    return 0;
}
