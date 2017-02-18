/*
 * $QNXLicenseC: 
 * Copyright 2007 - 2009, QNX Software Systems.  
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


#include <atomic.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <hw/i2c.h>
#include "imx35.h"


#define MCU_I2C_ADDR                    (0xD2 >> 1)
#define PMIC_I2C_ADDR			(0x68 >> 1)

#define MCU_RESET_CONTROL_2             0x1B
#define MCU_POWER_CONTROL               0x1C
#define MCU_GPIO_CONTROL_1              0x20


static int 
imx35_mcu_update (int fd, int8_t addr, uint8_t reg, uint8_t set, uint8_t reset)
{
	iov_t           siov[3], riov[2];
	i2c_sendrecv_t  srhdr;
	i2c_send_t      hdr;
	uint8_t		val = 0;

	srhdr.slave.addr = addr;
	srhdr.slave.fmt = I2C_ADDRFMT_7BIT;
	srhdr.send_len = 1;
	srhdr.recv_len = 1;
	srhdr.stop = 1;

	SETIOV(&siov[0], &srhdr, sizeof(srhdr));
	SETIOV(&siov[1], &reg, sizeof(reg));

	SETIOV(&riov[0], &srhdr, sizeof(srhdr));
	SETIOV(&riov[1], &val, 1);

	if (devctlv(fd, DCMD_I2C_SENDRECV, 2, 2, siov, riov, NULL) != EOK) {
		return -1;
	}

	val |= set;
	val &= ~reset;

	hdr.slave.addr = addr;
	hdr.slave.fmt = I2C_ADDRFMT_7BIT;
	hdr.len = 2;
	hdr.stop = 1;

	SETIOV(&siov[0], &hdr, sizeof(hdr));
	SETIOV(&siov[1], &reg, sizeof(reg));
	SETIOV(&siov[2], &val, 1);

	if (devctlv(fd, DCMD_I2C_SEND, 3, 0, siov, NULL, NULL) != EOK) {
		return -1;
	}

	return 0;
}

int
mcu_setup (disp_adapter_t *adapter, int enable)
{
	int	fd;

	if ((fd = open ("/dev/i2c0", O_RDWR)) == -1) {
		goto fail;
	}

	if (enable) {
		if (imx35_mcu_update (fd, MCU_I2C_ADDR, MCU_POWER_CONTROL, 0xf, 0)) {
			goto fail1;
		}

		if (imx35_mcu_update (fd, MCU_I2C_ADDR, MCU_GPIO_CONTROL_1, 0x40, 0)) {
			goto fail1;
		}
	} else {
		if (imx35_mcu_update (fd, MCU_I2C_ADDR, MCU_GPIO_CONTROL_1, 0, 0x40)) {
			goto fail1;
		}
	}

	close (fd);

	return (0);

	fail1:
		close (fd);
	fail:
		disp_printf(adapter,"devg-mx35: cannot enable/reset I2C part");

	return (-1);
}


