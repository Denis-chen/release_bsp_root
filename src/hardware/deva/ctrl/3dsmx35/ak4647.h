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

#ifndef __AK4647_H
#define __AK4647_H

/* I2C 7-bit slave address */
#define AK4647_SLAVE_ADDR_LO	0x12	/* CAD0=0 */
#define AK4647_SLAVE_ADDR_HI	0x13	/* CAD0=1 */

/*
 * AK4647 CODEC Registers
 */
#define POWER_MNGMT_1     0x00
#define POWER_MNGMT_2     0x01
#define SIGNAL_SLCT_1     0x02
#define SIGNAL_SLCT_2     0x03
#define MODE_CONTROL_1    0x04
#define MODE_CONTROL_2    0x05
#define TIMER_SELECT      0x06
#define ALC_MOD_CTRL_1    0x07
#define ALC_MOD_CTRL_2    0x08
#define LCH_IP_VOL_CTRL   0x09
#define LCH_DIG_VOL_CTRL  0x0A
#define ALC_MOD_CTRL_3    0x0B
#define RCH_IP_VOL_CTRL   0x0C
#define RCH_DIG_VOL_CTRL  0x0D
#define MODE_CONTROL_3    0x0E
#define MODE_CONTROL_4    0x0F
#define POWER_MNGMT_3     0x10
#define DIG_FILTER_SLCT   0x11

#endif /* __AK4647_H */

