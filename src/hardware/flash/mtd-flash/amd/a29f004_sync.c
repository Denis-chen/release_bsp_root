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




#include <pthread.h>
#include <sys/f3s_mtd.h>

/*
 * Summary
 *
 * MTD Version: 1 only
 * Bus Width:   8-bit and 16-bit
 * Boot-Block?: Yes
 * Note:        Converts f3s_a29f040_v2sync() to version 1 API
 *
 * Description
 *
 * This sync callout is for non-uniform (aka boot-block flash). This is the
 * only MTDv1 sync callout.
 */

int32_t f3s_a29f004_sync(f3s_dbase_t *dbase,
                         f3s_access_t *access,
                         uint32_t flags,
                         uint32_t offset,
                         int32_t size)
{
	uint32_t	i;
	uint32_t	temp;
	int			error;

	/* Just a subset of the corresponding v2 function */
	error = f3s_a29f040_v2sync(dbase, access, flags, offset);
	if (error) {
		if (error == EAGAIN) return (0);

		errno = error;
		return (-1);
	}

	/* Compute sector size */
	i    = 0;
	temp = 0;
	while (1) {
		temp += dbase->geo_vect[i].unit_num << dbase->geo_vect[i].unit_pow2;

		if (offset < temp) {
			return (1 << dbase->geo_vect[i].unit_pow2);
		}

		i++;
		if (i >= dbase->geo_num) i = 0;
	}

	/* If we ever get here, this is bad */
	fprintf(stderr, "(devf  t%d::%s:%d) inconsistant geometry. offset = 0x%x\n",
				pthread_self(), __func__, __LINE__, offset);
	errno = EIO;
	return (-1);
}

