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


/*
** File: generic_close.c
**
** Description:
**
** This file contains the close function for the generic flash library
**
** Ident: $Id: generic_close.c 169556 2008-06-02 12:43:32Z hsbrown $
*/

/*
** Includes
*/

#include "generic.h"

/*
** Function: generic_close
*/

void generic_close(f3s_socket_t *socket,
                     uint32_t flags)
{
	if (socket->memory) {
#if defined(__ARM__)
		munmap(socket->memory, socket->window_size);
#else
		munmap_device_memory(socket->memory, socket->window_size);
#endif		
	}
}

/*
** End
*/
