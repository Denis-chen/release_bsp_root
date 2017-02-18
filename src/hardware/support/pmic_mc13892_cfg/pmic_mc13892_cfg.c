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


#ifdef __USAGE
%C - MC13892 power management chip configuration utility via I2C bus.

#endif



#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <hw/inout.h>
#include <sys/mman.h>
#include <sys/resmgr.h>
#include <sys/neutrino.h>
#include <sys/rsrcdbmgr.h>
#include <hw/sysinfo.h>
#include <hw/i2c.h>

////////////////
// local vars //
////////////////

////////////////////////////////////////////////////////////////////////////////
//                            PRIVATE FUNCTIONS                               //
////////////////////////////////////////////////////////////////////////////////

/* mx35 write mc13892 */
void
mc13892_write(int i2c_fd, int slave_addr, unsigned char * databuf, int datalen)
{
	int i;
	struct {
		i2c_send_t      hdr;
		unsigned char   bytes[0];
	} *msg;

	msg = alloca(sizeof(*msg) + datalen);
	if (!msg) {
		perror("alloc failed");
		exit(-1);
	}

	msg->hdr.slave.addr = slave_addr;
	msg->hdr.slave.fmt = I2C_ADDRFMT_7BIT;
	msg->hdr.len = datalen;
	msg->hdr.stop = 1;

	for (i = 0; i < datalen; i++) {
		msg->bytes[i] = databuf[i];
	}

	if (devctl (i2c_fd, DCMD_I2C_SEND, msg, sizeof(*msg) + datalen, NULL))
		printf("MC13892 I2C_WRITE failed");
}

////////////////////////////////////////////////////////////////////////////////
//                                 MAIN                                       //
////////////////////////////////////////////////////////////////////////////////

int main( int argc, char *argv[] )
{
	int i2c_fd = 0;
	/* {# of bytes to send (including register address), register address, byte 0, byte 1, byte 2 } */
	unsigned char initdata[][10] = {
			{ 4, 0x1e, 0x02, 0x4d, 0xd3},		/* Set MC13892 Regulator Setting 0 Register, set VGEN voltage to 3.3v */
			{ 4, 0x20, 0x04, 0x92, 0x09},		/* Set MC13892 Regulator Mode 0, enable FEC 3v3 */
			{ 4, 0x21, 0,    0,    0x40}		/* Set MC13892 Regulator Mode 1, Bit 6 to enable VCAM */
			};

	// Enable IO capability.
	if( ThreadCtl( _NTO_TCTL_IO, NULL ) == -1 ) {
		perror( "ThreadCtl: " );
		exit( EXIT_FAILURE );
	}

	i2c_fd = open ("/dev/i2c0", O_RDWR);
	if (i2c_fd == -1)
	{
		printf ("MC13892 init: Failed to open I2C device /dev/i2c0");
		exit (-1);
	}

	/* set VGEN1 voltage to 3.3v, send {0x02, 0x4d, 0xd3} to register 30 */
	mc13892_write (i2c_fd, 0x8, &initdata[0][1], initdata[0][0]);

	/* Init power for FEC, send {0x04, 0x92, 0x09} to register 32 */
	mc13892_write (i2c_fd, 0x8, &initdata[1][1], initdata[1][0]);

	/* Init power for audio coded sgtl5000, send {0, 0, 0x40} to register 33 */
	mc13892_write (i2c_fd, 0x8, &initdata[2][1], initdata[2][0]);

	close (i2c_fd);

	exit( EXIT_SUCCESS );
}
