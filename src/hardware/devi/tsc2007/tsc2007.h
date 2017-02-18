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


#include <errno.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/neutrino.h>
#include <sys/time.h>
#include <hw/i2c.h>

#define TSC2007_PENIRQ	  68 

#define FLAG_INIT         0x1000
#define FLAG_RESET        0x2000

#define RELEASE_DELAY     100000000
#define INTR_DELAY        50
#define PULSE_PRIORITY    21

#define PULSE_CODE        1
#define NUMBER_OF_RETRY   (4)

#define JITTER_DELTA      10
#define TOUCH_DELTA       50

#define ABS(x)  ((x) >= 0 ? (x) : (-(x)))


// I2C controller device pathname
#define TSC_I2C_DEVICE		"/dev/i2c0"

// TSC2007 slave address
#define TSC_ADDRESS		0x48

// Speed of I2C communication with TSC. Currently use 100K - can try
// 400K later.
#define TSC_I2C_SPEED		100000

// 
// The TSC2007 is an I2C slave device that can operate in Standard, Fast, 
// and High speed mode. TSC2007 is controlled by the I2C master using a
// simple command based protocol. The first byte of data that follows the
// slave address byte is the command byte. The format of the command byte
// is as follows:
//
// ------------------------------------------------------------------
// |	D7	|	D6	|	D5	|	D4	|	D3	|	D2	|	D1	|	D0	|
// ------------------------------------------------------------------
//
//  Bit		Name		Description
//  D7-D4	C3-C0		Specifies command type. See below for details
//  D3-D2	PD1-PD0		00 - Power down between cycles. PENIRQ enabled.
//				01 - A/D converter on. PENIRQ disabled.
//				10 - A/D converter off. PENIRQ enabled.
//				11 - A/D converter on. PENIRQ disabled.
//  D1		M		0 - 12-bit data format
//				1 - 8-bit data format
//  D0		X		Don't care
//

// Define command types
#define	TSC_CMD_READTEMP0	(0<<4)		// Measure TEMP0
#define	TSC_CMD_SETUP		(11<<4)		// Setup command
#define TSC_CMD_READX		(12<<4)		// Measure X position
#define TSC_CMD_READY		(13<<4)		// Measure Y position

// Define power down modes
#define TSC_PWRDWN_PENIRQ_EN			(0<<2)
#define TSC_IREFOFF_ADCON_PENIRQ_DS		(1<<2)
#define TSC_IREFON_ADCOFF_PENIRQ_EN		(2<<2)
#define TSC_IREFON_ADCON_PENIRQ_DS		(3<<2)

// Define data format
#define TSC_FMT_12BIT	(0<<1)
#define TSC_FMT_8BIT	(1<<1)

// The Setup command format is different than other commands. The table
// below describes the Setup command byte.
//
//  Bit		Name				Description
//  D7-D4	C3-C0='1101'		Setup command
//  D3-D2	PD1-PD0='00'		Reserved; must be '00'
//  D1		Filter control		0 - use onboard MAV (default)
//					1 - bypass onboard MAV filter
//  D0		PENIRQ pullup		0 - R=50K ohm
//		resistor select		1 - R=90k ohm
//
#define TSC_PD_RESERVED		(0<<2)
#define TSC_FLTR_MAV_ON		(0<<1)
#define TSC_FLTR_MAV_OFF	(1<<1)

#define TSC_PNIRQ_PULLUP_50K	0
#define TSC_PNIRQ_PULLUP_90K	1			


typedef struct
{
	i2c_sendrecv_t	sr;
	uint8_t		data[2];
} tsc_sr_t;


static int tsc2007_init(input_module_t *module);
static int tsc2007_devctrl(input_module_t *module, int event, void *ptr);
static int tsc2007_reset(input_module_t *module);
static int tsc2007_pulse(message_context_t *, int, unsigned, void *);
static int tsc2007_parm(input_module_t *module,int opt,char *optarg);
static int tsc2007_shutdown(input_module_t *module, int delay);
static int read_data(unsigned int *x, unsigned int *y, void *data);
static void *intr_thread(void *data);

/* driver private data */
typedef struct _private_data
{
	int             irq;    /* IRQ to attach to */
	int             iid;    /* Interrupt ID */
	int             irq_pc; /* IRQ pulse code */

	int             chid;
	int             coid;
	pthread_attr_t       pattr;
	struct sched_param   param;
	struct sigevent      event;

	struct packet_abs tp;
	
	unsigned char	verbose;
	int		flags;

	unsigned        lastx, lasty;
	unsigned        touch_x, touch_y;

	pthread_mutex_t mutex;

	/* I2C related stuff */
	char		*i2c;
	int		fd;
	unsigned int	speed;	
	i2c_addr_t	slave;				

	/* Timer related stuff */
	timer_t         timerid;
	struct itimerspec itime;

	/* conversion params */
	long            release_delay;
	int             intr_delay;
	int             touch_delta;
	int		jitter_delta;
} private_data_t;
