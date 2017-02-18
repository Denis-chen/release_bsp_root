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


#include "mxcspi.h"


int mx35_wait(mx35_cspi_t *dev, int len)
{
	struct _pulse	pulse;

	while (1) {
		if (len) {
			uint64_t	to = dev->dtime;
			to *= len * 1000 * 50;	/* 50 times for time out */
			TimerTimeout(CLOCK_REALTIME, _NTO_TIMEOUT_RECEIVE, NULL, &to, NULL);
		}

		if (MsgReceivePulse(dev->chid, &pulse, sizeof(pulse), NULL) == -1)
			return -1;

		if (pulse.code == MX35_CSPI_EVENT)
			return 0;
	}

	return 0;
}
