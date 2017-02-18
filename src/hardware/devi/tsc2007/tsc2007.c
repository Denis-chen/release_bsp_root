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
 * tsc2007.c
 *
 * Driver for the TI TSC2007 low-power touch screen controller.
 * A Combination of Device and Protocol modules.
 */


#include <sys/devi.h>
#include "tsc2007.h"


input_module_t  tsc2007 = {
        NULL,                  /* up, filled in at runtime */
        NULL,                  /* down, filled in at runtime */
        NULL,                  /* line we belong to, filled in at runtime */
        0,                     /* flags, leave as zero */
        DEVI_CLASS_ABS | DEVI_MODULE_TYPE_PROTO | DEVI_MODULE_TYPE_DEVICE,
                               /* Our type, we are a
                                * protocol module or class
                                * relative. This info will
                                * tell the runtime system
                                * which filter module to
                                * link above us
                                */
        "tsc2007",             /* name, must match what you specify
                                * on the command line when invoking this
                                * module
                                */
        __DATE__,              /* date of compilation, used for output when
                                * the cmdline option -l is given to the
                                * driver
                                */
        "i:a:c:vd:D:p:j:t:",   /* command line parameters */
        NULL,                  /* pointer to private data, set this up
                                * in the init() callback
                                */
        tsc2007_init,          /* init() callback, required */
        tsc2007_reset,         /* reset() callback, required */
        NULL,                  /* input() callback */
        NULL,                  /* output(), not used */
        tsc2007_pulse,         /* pulse(), called when the timer expires
                                * used for injecting a release event
                                */
        tsc2007_parm,          /* parm() callback, required */
        tsc2007_devctrl,       /* devctrl() callback */
        tsc2007_shutdown       /* shutdown() callback, required */
};

static tsc_sr_t sndrcv;


/*
 * tsc2007_init()
 *
 * This is our init callback. Allocate space for our private data and assign
 * it to the data member of the module structure. Simple initialization.
 */
static int tsc2007_init(input_module_t *module)
{
	private_data_t *dp = module->data;

	if(!module->data)
	{
		if (!(dp = module->data = scalloc(sizeof *dp)))
		{
			return (-1);
		}
		
		ThreadCtl (_NTO_TCTL_IO, 0);

		dp->flags = FLAG_RESET;
		dp->irq = TSC2007_PENIRQ;
		dp->irq_pc = DEVI_PULSE_ALLOC;
		dp->lastx = 0;
		dp->lasty = 0;
		dp->param.sched_priority = PULSE_PRIORITY;
		dp->event.sigev_priority = dp->param.sched_priority;
		dp->release_delay = RELEASE_DELAY;
		dp->intr_delay = INTR_DELAY;
		dp->touch_delta = TOUCH_DELTA;
		dp->jitter_delta = JITTER_DELTA;
		dp->touch_x = 0;
		dp->touch_y = 0;

		dp->speed = TSC_I2C_SPEED;
		dp->slave.addr = TSC_ADDRESS;
		dp->slave.fmt = I2C_ADDRFMT_7BIT;

		dp->i2c = malloc(strlen(TSC_I2C_DEVICE) + 1);
		if (!dp->i2c)
		{
			slogf(_SLOGC_INPUT, _SLOG_ERROR, "%s: malloc failed for dev name", __FUNCTION__);
			free(dp);
			return (-1);
		}
		strcpy(dp->i2c, TSC_I2C_DEVICE);

		pthread_mutex_init (&dp->mutex, NULL);
	}
	return (0);
}


/* tsc2007_parm()
 *
 * Our callback to be called by the input runtime system to parse any
 * command line parameters given to the tsc2007 module.
 */
static int tsc2007_parm(input_module_t *module, int opt, char *optarg)
{
	private_data_t *dp = module->data;
	long int	v;

 	switch (opt)
	{
		case 'v':
				dp->verbose++;
				break;
		case 'i':
				dp->irq = atoi (optarg);
				break;
		case 'a':
				v = strtol(optarg, 0, 16);
				if (v > 0 && v < 128)
					dp->slave.addr = v;
				break;
		case 'c':
				if (strcmp(optarg, dp->i2c) != 0)
				{
					free(dp->i2c);
					dp->i2c = malloc(strlen(optarg) + 1);

					if (!dp->i2c)
					{
						slogf(_SLOGC_INPUT, _SLOG_ERROR, "%s: malloc failed for dev name", __FUNCTION__);
						return -1;
					}
					strcpy(dp->i2c, optarg);
				}
				break;
		case 'p':
				dp->param.sched_priority = atoi (optarg);
				dp->event.sigev_priority = dp->param.sched_priority;
				break;
		case 'd':
				dp->release_delay = (atol (optarg)) * 100000;   /* Convert to nsecs */
				break;
		case 'D':
				dp->intr_delay = atoi (optarg);
				break;
		case 'j':
				dp->jitter_delta = atoi (optarg);
				break;
		case 't':
				dp->touch_delta = atoi (optarg);
				break;
 		default:
				fprintf(stderr, "Unknown option %c\n", opt);
				break;
 	}

	return (0);
}


/* tsc2007_reset()
 *
 * The reset callback is called when our module is linked into an
 * event bus line. In here we will setup our device for action.
 *
 * We also create a timer, the purpose of this timer is to inject the
 * release events, since the TSC2007 touch controller does not give
 * any identification of a release.
 *
 * Also create a separate thread to handle the IRQs from the TSC2007.
 */
static int tsc2007_reset(input_module_t *module)
{
	private_data_t	*dp = module->data;
	int sts;

	if((dp->flags & FLAG_INIT) == 0)
	{
		/* Initialize I2C interface */
		dp->fd = open(dp->i2c, O_RDWR);
		if (dp->fd < 0)
		{
			slogf(_SLOGC_INPUT, _SLOG_ERROR, "%s: failure in opening I2C device %s", __FUNCTION__, dp->i2c);
			exit (-1);
		}
	
		sts = devctl(dp->fd, DCMD_I2C_SET_BUS_SPEED, &dp->speed, sizeof(dp->speed), NULL);
		if (sts != EOK)
		{
			slogf(_SLOGC_INPUT, _SLOG_ERROR, "%s: failed to set speed", __FUNCTION__);
			exit (-1);
		}

		/* setup */
		memset(&sndrcv, 0, sizeof(sndrcv));
		sndrcv.sr.slave = dp->slave;
		sndrcv.sr.stop = 1;
		sndrcv.sr.send_len = 1;
		sndrcv.sr.recv_len = 2;

		/* Create touch release timer */
		dp->timerid = devi_register_timer (module, 15, &dp->irq_pc, NULL);

		/* Setup the interrupt handler thread */
		if ((dp->chid = ChannelCreate (_NTO_CHF_DISCONNECT | _NTO_CHF_UNBLOCK)) == -1)
		{
			perror ("Error: ChannelCreate");
			exit (-1);
		}

		if ((dp->coid = ConnectAttach (0, 0, dp->chid, _NTO_SIDE_CHANNEL, 0)) == -1)
		{
			perror ("Error: ConnectAttach");
			exit (-1);
		}

		pthread_attr_init (&dp->pattr);
		pthread_attr_setschedpolicy (&dp->pattr, SCHED_RR);
		pthread_attr_setschedparam (&dp->pattr, &dp->param);
		pthread_attr_setinheritsched (&dp->pattr, PTHREAD_EXPLICIT_SCHED);
		pthread_attr_setdetachstate (&dp->pattr, PTHREAD_CREATE_DETACHED);
		pthread_attr_setstacksize (&dp->pattr, 4096);

		dp->event.sigev_notify     = SIGEV_PULSE;
		dp->event.sigev_coid       = dp->coid;
		dp->event.sigev_code       = 1;

		/* Create interrupt handler thread */
		if (pthread_create (NULL, &dp->pattr, (void *)intr_thread, module))
		{
			perror ("Error: pthread_create");
			exit (-1);
		}

		/* Attach interrupt. */
		if (dp->verbose >= 3)
			fprintf (stderr, "Attaching to interrupt %d\n", dp->irq);

		if ((dp->iid = InterruptAttachEvent (dp->irq, &dp->event, _NTO_INTR_FLAGS_TRK_MSK)) == -1) {
			perror ("Error: InterruptAttachEvent");
			exit (-1);
		}

		dp->flags |= FLAG_INIT;
	}
	return (0);
}


/*
 * tsc2007_devctrl()
 *
 * Our callback to be used by modules in an event bus line to send
 * information further up the line to other modules (e.g. abs). This
 * allows the other modules to know how many buttons we have, pointer
 * coordinates, and the range of the coordinates.
 */
static int tsc2007_devctrl(input_module_t *module, int event, void *ptr)
{
	private_data_t 	*dp = module->data;
	
	switch(event)
	{
		case DEVCTL_GETDEVFLAGS:
			*(unsigned short *)ptr = (dp->flags & FLAGS_GLOBAL);
			break;
		case DEVCTL_GETPTRBTNS:
			*(unsigned long *)ptr = 1L;
			break;
		case DEVCTL_GETPTRCOORD:
			*(unsigned char *)ptr = '\02';
			break;
		case DEVCTL_GETCOORDRNG:
		{
			struct devctl_coord_range *range = ptr;

			range->min = 0;
			range->max = 4096;
			break;
		}
		default:
			return (-1);
	}
	
	return (0);
}


/* This function is responsible for reading the touch data over I2C */
static int read_data (unsigned int *x, unsigned int *y, void *data)
{
	input_module_t    *module = (input_module_t *) data;
	private_data_t    *dp = module->data;
	int				sts;

	/* read X position with PENIRQ disabled */
	sndrcv.data[0] = TSC_CMD_READX | TSC_IREFON_ADCON_PENIRQ_DS | TSC_FMT_12BIT;
	sts = devctl(dp->fd, DCMD_I2C_SENDRECV, &sndrcv, sizeof(sndrcv), NULL);
	if (sts != EOK)
	{
		slogf(_SLOGC_INPUT, _SLOG_ERROR, "%s: devctl failed (sts=%d) for X", __FUNCTION__, sts);
		return -1;
	}

	*x = (sndrcv.data[0] << 4);
	*x += (sndrcv.data[1] >> 4);

	/* read Y position with PENIRQ disabled */
	sndrcv.data[0] = TSC_CMD_READY | TSC_IREFON_ADCON_PENIRQ_DS | TSC_FMT_12BIT;
	sts = devctl(dp->fd, DCMD_I2C_SENDRECV, &sndrcv, sizeof(sndrcv), NULL);
	if (sts != EOK)
	{
		slogf(_SLOGC_INPUT, _SLOG_ERROR, "%s: devctl failed (sts=%d) for Y", __FUNCTION__, sts);
		return -1;
	}
		
	*y = (sndrcv.data[0] << 4);
	*y += (sndrcv.data[1] >> 4);

	return (0);
}


/* Co-ordinate data is read from TSC2007 with PENIRQ disabled. This function
 * does a benign read to enable PENIRQ.
 */
static int enable_penirq (void * data)
{
	input_module_t    *module = (input_module_t *) data;
	private_data_t    *dp = module->data;
	int		sts;

	/* dummy read, enable PENIRQ */
	sndrcv.data[0] = TSC_CMD_READTEMP0 | TSC_IREFON_ADCOFF_PENIRQ_EN;
	sts = devctl(dp->fd, DCMD_I2C_SENDRECV, &sndrcv, sizeof(sndrcv), NULL);
	if (sts != EOK)
	{
		slogf(_SLOGC_INPUT, _SLOG_ERROR, "%s: devctl failed (sts=%d)", __FUNCTION__, sts);
		exit (-1);
	}

	return 0;
}

/* Do some basic processing on touch data to remove jitter.
 * Basically, this function "holds" successive co-ordinate values at the
 * first known reference of touch which we assume is accurate enough.
 */
static void process_data (void *data)
{
    input_module_t    *module = (input_module_t *) data;
    private_data_t    *dp = module->data;
	static uint16_t sample_x = 0;
	static uint16_t sample_y = 0;
	static int xcount = 0, ycount = 0;
	uint16_t touch_x, touch_y;

	touch_x = dp->tp.x;
	touch_y = dp->tp.y;

	/* first touch or genuine delta */
	if (((sample_x == 0) && (sample_y == 0)) ||
			((abs(sample_x - touch_x) > dp->jitter_delta) &&
			(abs(sample_y - touch_y) > dp->jitter_delta))) {
		sample_x = touch_x;
		sample_y = touch_y;
		xcount = ycount = 0;
		if (dp->verbose >= 4)
			fprintf(stderr, "TOUCH OR GENUINE DELTA X:%d Y:%d\n",
								touch_x, touch_y);
		return;
	}

	/* correct X jitter */
	if ((abs(sample_x - touch_x) < dp->jitter_delta) || xcount) {
		dp->tp.x = sample_x;
		xcount++;
		if (dp->verbose >= 4)
			fprintf(stderr, "JITTER CORRECTED X-DELTA:%d\n",
								abs(sample_x - touch_x));
	}

	/* correct Y jitter */
	if ((abs(sample_y - touch_y) < dp->jitter_delta) || ycount) {
		dp->tp.y = sample_y;
		ycount++;
		if (dp->verbose >= 4)
			fprintf(stderr, "JITTER CORRECTED Y-DELTA:%d\n",
								abs(sample_y - touch_y));
	}
}

/*
 * This is the code for the interrupt handler thread. It simply
 * waits on a pulse that is generated by the interrupt and then
 * requests the X and Y coordinates from the TSC2007 over I2C.
 *
 * Once the data has been fetched, a timer is started to inject
 * a release event. The timer will go off if no other interrupt
 * has been received, this is so we can simulate a release to the
 * upper layers.
 */
static void *intr_thread( void *data )
{ 
	input_module_t    *module = (input_module_t *) data;
	private_data_t    *dp = module->data;
	input_module_t    *up = module->up;
	struct _pulse   pulse;
	iov_t           iov;
	int             rcvid;
	unsigned int    x = 0, y = 0xFFF;
	int             i;

	SETIOV (&iov, &pulse, sizeof(pulse));

	while (1)
	{
		if ((rcvid = MsgReceivev (dp->chid, &iov, 1, NULL)) == -1)
		{
			if (errno == ESRCH)
			{
				pthread_exit (NULL);
			}

			continue;
		}

		switch (pulse.code)
		{
			case PULSE_CODE:
				/* Stop timer */
				dp->itime.it_value.tv_sec = 0;
				dp->itime.it_value.tv_nsec = 0;
				dp->itime.it_interval.tv_sec = 0;
				dp->itime.it_interval.tv_nsec = 0;
				timer_settime(dp->timerid, 0, &dp->itime, NULL);

				pthread_mutex_lock (&dp->mutex);

				for (i = 0; i < NUMBER_OF_RETRY; i++) {
					/* read data */
					read_data( &x, &y, data );

					if ((x == 0) && (y == 0xFFF)) {
						dp->tp.x = 0;
						dp->tp.y = 0xFFF;
						continue;
					}

					dp->tp.buttons = _POINTER_BUTTON_LEFT;

					if (dp->verbose >= 4)
						fprintf(stderr, "PRE FILTER  X:%d Y:%d\n", x, y);

					/**
					 * Accept the input if one of the following criteria met:
					 * 1. the touch is released
					 * 2. the touch is within range
					 */
					if ((dp->lastx == 0 && dp->lasty == 0)
					   || ((abs (dp->lastx - x) <= dp->touch_delta)
					      && (abs (dp->lasty - y) <= dp->touch_delta))) {
						if (dp->verbose >=2)
							fprintf(stderr, "ACCEPTED    X:%d Y:%d\n", x, y);

						dp->tp.x = x;
						dp->tp.y = y;

						if (i > 0 && i < NUMBER_OF_RETRY) {
							break;
						}
					} else {
						if (dp->verbose >= 2)
							fprintf(stderr, "REJECTED    X:%d Y:%d Diff x=%d y=%d\n", x, y, abs (dp->lastx-x), abs (dp->lasty-y));

						dp->tp.x = 0;
						dp->tp.y = 0xFFF;
					}

					dp->lastx = x;
					dp->lasty = y;
				}

				/* Check for garbage packets */
				if ((dp->tp.x != 0) && (dp->tp.y != 0xFFF))
				{
					/* preprocess data for removing jitter */
					process_data(data);

					if (dp->verbose >= 3)
						fprintf(stderr, "X:%d Y:%d Touched\n", dp->tp.x, dp->tp.y);
					dp->touch_x = dp->tp.x;
					dp->touch_y = dp->tp.y;

					clk_get(&dp->tp.timestamp);
					(up->input)(up, 1, &dp->tp);
				}

				pthread_mutex_unlock (&dp->mutex);

				delay (dp->intr_delay);

				/* (Re)Start timer */
				dp->itime.it_value.tv_sec = 0;
				dp->itime.it_value.tv_nsec = dp->release_delay;
				dp->itime.it_interval.tv_sec = 0;
				dp->itime.it_interval.tv_nsec = 0;
				timer_settime(dp->timerid, 0, &dp->itime, NULL);

				enable_penirq(data);
				InterruptUnmask (dp->irq, dp->iid);
				break;

			default:
				if (rcvid) {
					MsgReplyv (rcvid, ENOTSUP, &iov, 1);
				}
				break;
			}
		}

	return( NULL );
}


/*
 * tsc2007_pulse()
 *
 * This is the callback for event notifications from the input runtime
 * system. In our case, this is the timer pulse handler that gets called
 * when the timer has expired. This simply injects a release event into
 * the abs module.
 */
static int tsc2007_pulse (message_context_t *ctp, int code, unsigned flags, void *data)
{
	input_module_t    *module = (input_module_t *) data;
	input_module_t    *up = module->up;
	private_data_t    *dp = module->data;

	pthread_mutex_lock (&dp->mutex);

	dp->tp.x = dp->touch_x;
	dp->tp.y = dp->touch_y;
	dp->tp.buttons = 0;

	if (dp->verbose >= 3)
		fprintf(stderr, "X:%d Y:%d Released\n", dp->tp.x, dp->tp.y);

	clk_get(&dp->tp.timestamp);
	(up->input)(up, 1, &dp->tp);

	pthread_mutex_unlock (&dp->mutex);

	InterruptUnmask (dp->irq, dp->iid);

	return (0);
}


/*
 * tsc2007_shutdown()
 *
 * This callback performs the cleanup of the driver when the input
 * runtime system is shutting down.
 */
static int tsc2007_shutdown(input_module_t *module, int shutdown_delay)
{
	private_data_t  *dp = module->data;

	delay (shutdown_delay);

	close(dp->fd);
	free(dp->i2c);

	free (module->data);

	return (0);
}
