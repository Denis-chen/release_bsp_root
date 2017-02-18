/*
 * $QNXLicenseC: 
 * Copyright 2008, QNX Software Systems.  
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

#include <hw/i2c.h>

#include "mx35_3ds.h"
#include "ak4647.h"

/* I2C device on i.MX35 board */
#define MX35_3DS_AK4647_I2C_DEV		"/dev/i2c0"
#define MX35_3DS_AK4647_SLAVE_ADDR	AK4647_SLAVE_ADDR_LO

int
mx35_3ds_i2c_init (HW_CONTEXT_T * mx35_3ds)
{
	int fd;

	fd = open (MX35_3DS_AK4647_I2C_DEV, O_RDWR);
	if (fd == -1)
	{
		ado_debug (DB_LVL_DRIVER | DB_LVL_CONTROL,
		    "I2C_INIT: Failed to open I2C device");
		return -1;
	}

	mx35_3ds->i2c_fd = fd;
	return 0;
}

uint8_t
mx35_3ds_i2c_read (HW_CONTEXT_T * mx35_3ds, uint8_t reg)
{
    iov_t           siov[2], riov[2];
    i2c_sendrecv_t  srhdr;
	uint8_t			val = 0;
	int				fd = mx35_3ds->i2c_fd;

    srhdr.slave.addr = MX35_3DS_AK4647_SLAVE_ADDR;
    srhdr.slave.fmt = I2C_ADDRFMT_7BIT;
    srhdr.send_len = 1;
    srhdr.recv_len = 1;
    srhdr.stop = 1;

    SETIOV(&siov[0], &srhdr, sizeof(srhdr));
    SETIOV(&siov[1], &reg, sizeof(reg));

    SETIOV(&riov[0], &srhdr, sizeof(srhdr));
    SETIOV(&riov[1], &val, 1);

	if (devctlv(fd, DCMD_I2C_SENDRECV, 2, 2, siov, riov, NULL) == EOK)
    {
		ado_debug (DB_LVL_MIXER, "AK4647: Read [0x%02x] = 0x%02x", reg, val);
	}
	else
	{
		ado_debug (DB_LVL_DRIVER, "AK4647 I2C_READ failed");
		val = 0;
	}

	return val;	
}

void
mx35_3ds_i2c_write (HW_CONTEXT_T * mx35_3ds, uint8_t reg, uint8_t val)
{
    iov_t           siov[3];
    i2c_send_t      hdr;
	int				fd = mx35_3ds->i2c_fd;

    hdr.slave.addr = MX35_3DS_AK4647_SLAVE_ADDR;
    hdr.slave.fmt = I2C_ADDRFMT_7BIT;
    hdr.len = 2;
    hdr.stop = 1;

    SETIOV(&siov[0], &hdr, sizeof(hdr));
    SETIOV(&siov[1], &reg, sizeof(reg));
    SETIOV(&siov[2], &val, 1);

    if (devctlv(fd, DCMD_I2C_SEND, 3, 0, siov, NULL, NULL) != EOK)
		ado_debug (DB_LVL_DRIVER, "AK4647 I2C_WRITE failed");
	else
		ado_debug (DB_LVL_MIXER, "AK4647: Wrote [0x%02x] = 0x%02x", reg, val);
}
