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


#include "generic.h"


int32_t
generic_open(f3s_socket_t *socket, uint32_t flags)
{
#if defined(__ARM__)
	int		fd;
	char	buf[16];
#endif
	if (socket->memory)
		return EOK;

	ThreadCtl(_NTO_TCTL_IO, 0);

	if (f3s_socket_option(socket)) {
		printf("You must use the -s option to pass in base addr and window size\n");
		exit(1);		
	}
	socket->name = (uint8_t *) "generic";

#if defined(__ARM__)
	/*
	 * Create shm object for mapping flash memory
	 */
	sprintf(buf, "flash%llx", socket->address);
	if ((fd = shm_open(buf, O_RDWR | O_CREAT, 0666)) == -1) {
		perror("shm_open");
		return -1;
	}
	unlink(buf);

	/*
	 * Set the shm object address and size
	 */
	if (shm_ctl(fd, SHMCTL_PHYS|SHMCTL_PRIV, socket->address, socket->window_size) < 0) {
		perror("shm_ctl");
		close(fd);
		return -1;
	}

	socket->memory = mmap(0, socket->window_size,
			PROT_READ|PROT_WRITE|PROT_NOCACHE,
			MAP_SHARED, fd, 0);
	close(fd);
#else
	socket->memory = mmap_device_memory(0, socket->window_size,
						  PROT_READ|PROT_WRITE|PROT_NOCACHE,
						  0, socket->address);
#endif

	if (socket->memory == MAP_FAILED) {
		return -1;
	}
	return EOK;
}
