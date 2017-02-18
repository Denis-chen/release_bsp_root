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


#include "proto.h"

uint32_t
mx35_wait_status(mx35_dev_t *dev)
{
	uint16_t	status;
	uint64_t	ntime = 0x1000;
	int			interr = EOK;

	while ( interr != ETIMEDOUT ){
		TimerTimeout(CLOCK_MONOTONIC, _NTO_TIMEOUT_INTR, NULL, &ntime, NULL);
		interr = InterruptWait_r(0, NULL);
		status = in16(dev->regbase + MX35_I2C_STSREG_OFF);
		if (status & STSREG_IIF){
			out16(dev->regbase + MX35_I2C_STSREG_OFF, in16(dev->regbase + MX35_I2C_STSREG_OFF) & ~(STSREG_IIF));
			InterruptUnmask(dev->intr, dev->iid);
			return status;
		}
	}
	return 0;
}

i2c_status_t
mx35_recvbyte(mx35_dev_t *dev, uint8_t *byte, int nack, int stop){
	uint32_t	status;

	status = mx35_wait_status(dev);

	if (!(status & STSREG_ICF)){
		out16(dev->regbase + MX35_I2C_CTRREG_OFF, CTRREG_IEN | CTRREG_IIEN);
		return I2C_STATUS_ERROR;
	}

	if (nack){
		out16(dev->regbase + MX35_I2C_CTRREG_OFF, CTRREG_IEN | CTRREG_IIEN | CTRREG_MSTA | CTRREG_TXAK);
	}else if(stop){
		out16(dev->regbase + MX35_I2C_CTRREG_OFF, CTRREG_IEN | CTRREG_TXAK);
	}

	*byte = in16(dev->regbase + MX35_I2C_DATREG_OFF) & 0xFF;
	return 0;
}

i2c_status_t
mx35_sendbyte(mx35_dev_t *dev, uint8_t byte){
	uint32_t status;

	out16(dev->regbase + MX35_I2C_CTRREG_OFF, in16(dev->regbase + MX35_I2C_CTRREG_OFF) | CTRREG_MTX);
	out16(dev->regbase + MX35_I2C_DATREG_OFF, byte);

	status = mx35_wait_status(dev);

	if (!(in16(dev->regbase + MX35_I2C_CTRREG_OFF) & CTRREG_MSTA)){
		if (status & STSREG_IAL) {
			out16(dev->regbase + MX35_I2C_STSREG_OFF, in16(dev->regbase + MX35_I2C_STSREG_OFF) & ~STSREG_IAL);
			return I2C_STATUS_ARBL;
		}
		if (status & STSREG_IAAS)
			return I2C_STATUS_ERROR;
	}

	if (status & STSREG_RXAK){
		out16(dev->regbase + MX35_I2C_CTRREG_OFF, CTRREG_IEN);
		return I2C_STATUS_NACK;
	}
	return 0;
}

i2c_status_t
mx35_sendaddr7(mx35_dev_t *dev, unsigned addr, int read, int restart){
	out16(dev->regbase + MX35_I2C_CTRREG_OFF, CTRREG_IEN | CTRREG_IIEN | CTRREG_MSTA |
												(restart? CTRREG_RSTA: 0) | CTRREG_MTX);
	while(!(in16(dev->regbase + MX35_I2C_STSREG_OFF) & STSREG_IBB));

	return mx35_sendbyte(dev, (addr << 1) | read);
}

i2c_status_t
mx35_sendaddr10(mx35_dev_t *dev, unsigned addr, int read, int restart){
	i2c_status_t	err;

	out16(dev->regbase + MX35_I2C_CTRREG_OFF, CTRREG_IEN | CTRREG_IIEN | CTRREG_MSTA |
												(restart? CTRREG_RSTA: 0) | CTRREG_MTX);
	while(!(in16(dev->regbase + MX35_I2C_STSREG_OFF) & STSREG_IBB));

	if (err = mx35_sendbyte(dev, MX35_I2C_XADDR1(addr)))
		return err;
	if (err = mx35_sendbyte(dev, MX35_I2C_XADDR2(addr)))
		return err;

	if (read){
		out16(dev->regbase + MX35_I2C_CTRREG_OFF, CTRREG_IEN | CTRREG_IIEN | CTRREG_MSTA |
													CTRREG_RSTA | CTRREG_MTX);
		if (err = mx35_sendbyte(dev, MX35_I2C_XADDR1(addr) | read))
			return err;
	}
	return 0;
}
