/*
 * $QNXLicenseC:
 * Copyright 2009, QNX Software Systems.
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
/**
 * @file
 * @brief Touch screen driver for the MC13892 used on the i.MX35 PDK
 *
 * @mainpage
 *
 * This driver uses a combination of interrupts and hardware polling.
 * When the driver is started the MC13892 is put in interrupt mode.  In this
 * mode the MC13892 waits for contact between the touch plates.  When a contact
 * (touch) is sensed the MC13982 generates an interrupt.  The driver receives
 * the interrupt
 * - Puts the MC13892 in position mode (touch mode in MC13892 docs) 
 * - Requests the coordinate and resistance data from the ADC
 * - Does some simple processing and validation of the data 
 * - Injected good coordinates into the input framework.  
 * - Then switches into hardware polling mode until a release event.  
 * - A release event is denoted by a contact resistance below a
 * programmable threshold.
 *
 */

#include <sys/devi.h>
#include <hw/i2c.h>
#include "touch.h"

/**
 * Command line parameters
 *
 *  -i irq           
 *      - IRQ for sample device (default 96).
 *
 *  -a device
 *      - Sample device control interface default (/dev/i2c0).
 *
 *  -b speed         
 *      - Sample device control interface speed default (100000).
 *
 *  -p priority      
 *      - Pulse priority for the interrupt handling thread (default 21).
 *
 *  -D delay         
 *      - Millisecond minimum delay between interrupts (default 75)
 *
 *  -d delay         
 *      - Millisecond delay timer for injected events (default 100).
 *
 *  -t               
 *      - Touch threshold presure, resistance  0-1024 (default xx)
 *
 *  -v               
 *      - Verbosity, added v's means more verbosit
 *
 *
 */
#define CMD_PARAMETERS "i:a:b:p:D:d:t:v:"

/**
 *  structure containing state of touch screen driver
 */
typedef struct _private_data {
	int irq; /**< IRQ to attach to */
	int iid; /**< Interrupt ID */
	int irq_pc; /**< IRQ pulse code */

	int chid; /**< Interrupt channel ID */
	int coid; /**< Interrupt connection ID */
	pthread_attr_t pattr; /**< Interrupt thread attributes  */
	struct sched_param param; /**< Scheduling parameter for interrupt thread */
	struct sigevent event; /**< Interrupt event */

	/**
	 * Hardware control I2C device name
	 */
	char *hw_device;
	/**
	 * I2C bus speed
	 */
	unsigned hw_bus_speed;
	/**
	 * Context data for control interface I2C.
	 * Contains the file descriptor for the I2C device connection.
	 */
	uintptr_t hw_ctx;
   
	/** touch event packet 
	 * Packet sent to to input runtime for touch event.
	 */
	struct packet_abs tp;

	unsigned char verbose; /**< Verbose level set using multiple -v on command line (-vvv) */
	int flags; /**< Driver state flags */

	unsigned lastx, /**< Last valid touch x coordinate */
	lasty; /**< Last valid touch y coordinate */

	pthread_mutex_t mutex; /** Mutex to manage concurrent access to hardware */

	/* Timer related stuff */
	timer_t timerid; /**< Touch release timer identifier  */
	struct itimerspec itime; /**< Touch relase time specification */

	long intr_delay; /**< Minimum interrupt delay.  */
	long poll_delay; /**< Poll delay.  The time between hardware polls, controls the event injection rate.  Lager delays cause slower injection rates */
	int touch_threshold; /**< Restance required to be considered a touch 0-1024 */

} private_data_t;

/*
 * Pre-declared functions
 */
static int touch_init(input_module_t * module);
static int touch_devctrl(input_module_t * module, int event, void *ptr);
static int touch_reset(input_module_t * module);
static int touch_pulse(message_context_t *, int, unsigned, void *);
static int touch_parm(input_module_t * module, int opt, char *optarg);
static int touch_shutdown(input_module_t * module, int delay);
static void *intr_thread(void *data);
static int i2c_read(uint32_t base, int reg);
static void i2c_write(uint32_t base, int reg, int val);
static void set_touchscreen_mode(void *data, int mode);
static int process_data(uint16_t raw_x[2], uint16_t raw_y[2],
		uint16_t raw_r[2], int *x, int *y, void *data);

/**
 *	Touch screen input module.
 *	We create one input_module_t structure to represent the touch screen.
 *	@note If more than one are needed, i.e. in multiple bus lines; then the system
 *	will allocate a new module and copy the contents of the static one
 *	into it.
 */
input_module_t touch = {
	NULL,           /* up, filled in at runtime */
	NULL,           /* down, filled in at runtime */
	NULL,           /* line we belong to, filled in at runtime */
	0,              /* flags, leave as zero */
	DEVI_CLASS_ABS | DEVI_MODULE_TYPE_PROTO | DEVI_MODULE_TYPE_DEVICE,
  	                /* Our type, we are a
 	                 * protocol module or class
 	                 * relative.  This info will
	                 * tell the runtime system
	                 * which filter module to
	                 * link above us
 	                 */
	"touch",        /* name, must match what you specify
	                 * on the command line when invoking this
	                 * module
	                 */
	__DATE__,       /* date of compilation, used for output when
				        * the cmdline option -l is given to the
 				        * driver
 				        */
	CMD_PARAMETERS, /* command line parameters */
	NULL,           /* pointer to private data, set this up
 		 	           * in the init() callback
 			           */
	touch_init,     /* init() callback, required */
	touch_reset,    /* reset() callback, required */
	NULL,           /* input() callback */
	NULL,           /* output(), not used */
	touch_pulse,    /* pulse(), called when the timer expires
 	                 * used for injecting a release event */
	touch_parm,     /* parm() callback, required */
	touch_devctrl,  /* devctrl() callback */
	touch_shutdown  /* shutdown() callback, required */
};

/**
 * Initialization function.
 * Called by the input driver to initialize this driver.
 * @param module Module to initialize,  The actual touch module or a copy of it.
 * @return 0 always success
 * @note Callback specified in the input_module_t structure
 *
 */
static int touch_init(input_module_t * module) {
	private_data_t *dp = module->data;
	TRACE;

	if (!module->data) {
		if (!(dp = module->data = scalloc(sizeof *dp))) {
			return (-1);
		}
		ThreadCtl(_NTO_TCTL_IO, 0);

		dp->flags = FLAG_RESET;
		dp->irq = TOUCH_INT;
		dp->irq_pc = DEVI_PULSE_ALLOC;
		dp->hw_device = MC13892_I2C_DEVICE;
		dp->hw_bus_speed = MC13892_I2C_BUS_SPEED;
		dp->lastx = 0;
		dp->lasty = 0;
		dp->param.sched_priority = PULSE_PRIORITY;
		dp->event.sigev_priority = dp->param.sched_priority;
		dp->intr_delay = INTR_DELAY;
		dp->poll_delay = HW_POLL_TIME;
		dp->touch_threshold = MC13892_TOUCH_RESISTANCE_DEFAULT;
		pthread_mutex_init(&dp->mutex, NULL);
	}
	return (0);
}
/* Doxygen, another way to document function parameters */
/**
 * Parses the driver command line options.
 * Called once for each option.
 * @note called by input runtime.
 */
static int touch_parm(
	input_module_t * module,  /**< Module structure representing this instance of the driver */
	int opt,                  /**< Options to parse */ 
	char *optarg              /**< Argument for the option */
	) {
	private_data_t *dp = module->data;
	TRACE;

	switch (opt) {
	   /* verbosity */
	case 'v':
		dp->verbose++;
		break;
		/* interrupt */
	case 'i':
		dp->irq = atoi(optarg);
		break;
		/* Hardware control device name */
	case 'a':
		dp->hw_device = optarg;
		break;
		/* Hardware control device speed */
	case 'b':
		dp->hw_bus_speed = atol(optarg);
		break;
		/* priority */
	case 'p':
		dp->param.sched_priority = atoi(optarg);
		dp->event.sigev_priority = dp->param.sched_priority;
		break;
		/* interrupt delay, used to slow interrupts */
	case 'D':
		dp->intr_delay = atoi (optarg);
		DEBUG_CMD(slogf(99,1,"Interrupt delay %d", dp->intr_delay);)
		break;
		/* delay time for hardware poll and therefor event injection */
	case 'd':
		dp->poll_delay = (atol(optarg)) * 100000; /* Convert to nsecs */
		DEBUG_CMD(slogf(99,1,"Poll delay %d", dp->poll_delay);)
		break;
		/* touch resistance threshold */
	case 't':
		dp->touch_threshold = atoi(optarg);
		break;
	default:
		fprintf(stderr, "Unknown option -%c", opt);
		break;
	}

	return (0); /** @return  0 always successful */
}

/**
 * Reset the module.
 *  - Configure  hardware control interface 
 *  - Create a timer to timeout touch events and inject the release events,
 *  - Create a separate thread to handle the IRQ's from the touch controller.
 * @param module Module structure representing this instance of the driver
 * @return 0 always returns success, calls exit(-1) on fail condition.
 * @note called by input runtime.
 */
static int touch_reset(input_module_t * module) {
	private_data_t *dp = module->data;
	uint32_t buf = 0;
	TRACE;

	if ((dp->flags & FLAG_INIT) == 0) {
		int status;
		// Enable IO capability.
		if (ThreadCtl(_NTO_TCTL_IO, NULL) == -1) {
			perror("ThreadCtl: ");
			exit(EXIT_FAILURE);
		}
		dp->hw_ctx = open(dp->hw_device, O_RDWR);
		if (dp->hw_ctx == -1) {
			printf("Failed to open I2C device %s for MC13892", dp->hw_device);
			exit(-1);
		}

		if (status = devctl(dp->hw_ctx, DCMD_I2C_SET_BUS_SPEED,
				&(dp->hw_bus_speed), sizeof(dp->hw_bus_speed), NULL)) {
			errno = status;
			perror("devctl(BUS_SPEED)");
		}

		/* Clear MC13892 interrupt status
		 * Bits 0 : ADCDONE
		 * Bits 2 : TSI touch screen wake upe
		 */
		i2c_write(dp->hw_ctx, INT_STATUS_0_REG, (1 << 0) | (1 << 2));

		buf = i2c_read(dp->hw_ctx, INT_MASK_0_REG);
		if (dp->verbose >= 3) {
			fprintf(stderr, "Mask Reg: %x\n", buf);
		}
		/* clear TSI bit 4 in MC13892 mask register */
		buf = ~MC13892_TSI_BIT;
		i2c_write(dp->hw_ctx, INT_MASK_REG, buf);

		/* Setup the PLLX timer
		 In switcher register 4 address 28
		 bit 18 in PLLEN set 1-enable PLL
		 Bits 19 PLL0  set 1
		 Bits 20 PLL0  set 1
		 Bits 21 PLL2  set 1
		 Frequency is 3440640Hz the maximum
		 */
		buf = i2c_read(dp->hw_ctx, TIMER_REG);
		buf &= ~((1<<18) | (1<<19) |  (1<<20) | (1<<21));
		buf |= ((1<<18) | (1<<19) |  (1<<20) | (1<<21));
		i2c_write(dp->hw_ctx, TIMER_REG, buf);
		/* Put PMIC touch ADC into interrupt mode (required for pen down interrupt) */
		set_touchscreen_mode(module, INTERRUPT_MODE);

		/* Create touch release timer */
		dp->timerid = devi_register_timer(module, 15, &dp->irq_pc, NULL);

		/* Setup the interrupt handler thread */
		if ((dp->chid = ChannelCreate(_NTO_CHF_DISCONNECT | _NTO_CHF_UNBLOCK))
				== -1) {
			perror("Error: ChannelCreate");
			exit(-1);
		}

		if ((dp->coid = ConnectAttach(0, 0, dp->chid, _NTO_SIDE_CHANNEL, 0))
				== -1) {
			perror("Error: ConnectAttach");
			exit(-1);
		}

		pthread_attr_init(&dp->pattr);
		pthread_attr_setschedpolicy(&dp->pattr, SCHED_RR);
		pthread_attr_setschedparam(&dp->pattr, &dp->param);
		pthread_attr_setinheritsched(&dp->pattr, PTHREAD_EXPLICIT_SCHED);
		pthread_attr_setdetachstate(&dp->pattr, PTHREAD_CREATE_DETACHED);
		pthread_attr_setstacksize(&dp->pattr, 4096);

		dp->event.sigev_notify = SIGEV_PULSE;
		dp->event.sigev_coid = dp->coid;
		dp->event.sigev_code = 1;

		/* Attach interrupt. */
		if (dp->verbose >= 3) {
			fprintf(stderr, "Attaching to interrupt %d\n", dp->irq);
		}
		if ((dp->iid = InterruptAttachEvent(dp->irq, &dp->event,
				_NTO_INTR_FLAGS_TRK_MSK)) == -1) {
			perror("Error: InterruptAttachEvent");
			exit(-1);
		}

		/* Create interrupt handler thread */
		if (pthread_create(NULL, &dp->pattr, (void *) intr_thread, module)) {
			perror("Error: pthread_create");
			exit(-1);
		}

		dp->flags |= FLAG_INIT;
	}

	TRACE_EXIT;
	return (0);
}

/**
 * Informs input runtime about device capabilities.
 * The number of buttons, number of coordinates and range of coordinates
 * @param module Module structure representing this instance of the driver
 * @param event Input event type
 * @param ptr Pointer to data structure to write ctrl value into.
 * @return 0 always returns success, calls exit(-1) on fail condition.
 * @note Called by input runtime.
 * @note Used by modules in an event bus line to send information further up
 * the line to other modules (e.g. abs).
 */
static int touch_devctrl(input_module_t * module, int event, void *ptr) {
	private_data_t *dp = module->data;
	TRACE;

	switch (event) {
	case DEVCTL_GETDEVFLAGS:
		*(unsigned short *) ptr = (dp->flags & FLAGS_GLOBAL);
		break;
	case DEVCTL_GETPTRBTNS:
		*(unsigned long *) ptr = 1L;
		break;
	case DEVCTL_GETPTRCOORD:
		*(unsigned char *) ptr = (unsigned char) 2;
		break;
	case DEVCTL_GETCOORDRNG: {
		struct devctl_coord_range *range = ptr;

		range->min = 0;
		range->max = 1024;
		break;
	}
	default:
		return (-1);
	}

	TRACE_EXIT;
	return (0);
}


/**
 * Set the touch screen mode.
 * Sets the touch screen mode for the MC13892.
 * - Interrupt mode, the MC13892 waits for a touch event i.e. generates an interrupt
 *   when the plates make contact.
 * - Position mode, the MC13892 is reading positional data from ADC.
 * - Inactive mode, the MC13892 touch screen inactive touchscreen input can be used
 *   for general purpose ADC input.
 * @param data device context (module_t)
 * @param mode Mode to set (INTERRUPT/POSITION/INACTIVE)
 */
static void set_touchscreen_mode(void *data, int mode) {
	input_module_t *module = (input_module_t *) data;
	private_data_t *dp = module->data;
	uint32_t buf;
	TRACE;
	buf = i2c_read(dp->hw_ctx, ADC0_REG);
	buf &= ~MC13892_MODE_MASK;

	if (mode == POSITION_MODE) {
		DEBUG_MSG("POSITION_MODE");
		buf = MC13892_TOUCHSCREEN_MODE;
	} else if (mode == INTERRUPT_MODE) {
		DEBUG_MSG("INTERRUPT_MODE");
		if (dp->verbose) {
			fprintf(stderr, "INTERRUPT MODE!!!\n");
		}
		buf = MC13892_INTERRUPT_MODE;
	} else {
		DEBUG_MSG("INACTIVE_MODE");
		buf = MC13892_INACTIVE_MODE;
	}

	i2c_write(dp->hw_ctx, ADC0_REG, buf);
}

/**
 * Start coordinate conversion.
 * - Setup ADC for touchscreen conversion.
 * - Set auto increment mode
 * - Start ADC convertion 
 * - Loop Waiting for conversion to finish (max 13ms)
 * 
 * Called in Position mode
 * @param data device context (module_t)
 * @note  called by interrupt handler thread.
 */
void start_conversion(void *data) {
	input_module_t *module = (input_module_t *) data;
	private_data_t *dp = module->data;
	uint32_t buf = 0;
	int i;
	TRACE;

	/*
	 * Setup ADC for touchsreen conversion.
	 * Reg ADC1
	 * bit 0 ADEN    - 1 enable ADC
	 * bit 1 RAND    - 0?
	 * bit 3 ADSEL   - 1 set to touch screen
	 * bit 11-18 AT0 - 0x01 delay conversion (default from 13783)
	 * bit 19 ATOX   - 0 delay before first only
	 * bit 21 ADTRIG - 0 Ignore ADTRIG
	 */
	buf = i2c_read(dp->hw_ctx, ADC1_REG);
	buf &= ~((1 << 21) | (0 << 19) | (0x0f << 11) | (1 << 3) | (0 << 1) | (1
			<< 0));
	buf |= (0 << 21) | (0 << 19) | (0x01 << 11) | (1 << 3) | (0 << 1)
			| (1 << 0);
	i2c_write(dp->hw_ctx, ADC1_REG, buf);

	// increment mode.
	/*
	 * Reg ADC0
	 * bit 10 TSREF   - 1 Enable touch screen reference
	 * bit 16 ADINC1  - 1 Enable read auto increment ADA1
	 * bit 17 ADINC2  - 1 Enable read auto increment ADA2
	 */
	buf = i2c_read(dp->hw_ctx, ADC0_REG);
	buf &= ~((1 << 10) | (1 << 16) | (1 << 17));
	buf |= ((1 << 10) | (1 << 16) | (1 << 17));
	i2c_write(dp->hw_ctx, ADC0_REG, buf);

	/*
	 * Set auto increment start values
	 * Reg ADC1
	 * bit 5-7  ADA1 - 0x0  Start value
	 * bit 8-10 ADA2 - 0x0  Start value
	 */
	buf = i2c_read(dp->hw_ctx, ADC1_REG);
	buf &= ~((0x7 << 5) | (0x7 << 8));
	buf |= ((0x0 << 5) | (0x0 << 8));
	i2c_write(dp->hw_ctx, ADC1_REG, buf);

	// Start the ADC conversion
	/*
	 * Reg ADC1
	 * bit 20 ASC - 1 start conversion
	 */
	buf = i2c_read(dp->hw_ctx, ADC1_REG);
	buf &= ~(1 << 20);
	buf |= (1 << 20);
	i2c_write(dp->hw_ctx, ADC1_REG, buf);

	for (i = 0; i < 13; i++) {
		buf = i2c_read(dp->hw_ctx, INT_STATUS_0_REG);

		if (buf & ADCDONE1) {
			// Clear ADCDONE1, unmask and continue
			buf |= ~ADCDONE1;
			i2c_write(dp->hw_ctx, INT_STATUS_0_REG, buf);
			buf = i2c_read(dp->hw_ctx, 1);
			buf &= ADCDONE1_MASK;
			break;
		}
		delay(1);
	}

	if (i == 13) {
		fprintf(stderr, "Limit Reached conversion did not complete.\n");
		fprintf(stderr, "Interrupt Reg: %x\n", i2c_read(dp->hw_ctx,
				INT_STATUS_0_REG));
	}
}

/**
 * Read coordinate information from MC13892 ADC.
 * In order to reduce the interrupt rate and to allow for easier noise rejection,
 * the touch screen readings are repeated in the readout sequence.
 *     		ADC Conversion       Signals sampled       Readout
 *        		Address (*)
 *         		0                X position           000
 *					1                X position           001
 *					2                 Dummy               010
 *					3                Y position           011
 *					4                Y position           100
 *					5                 Dummy               101
 *					6            Contact resistance       110
 *					7            Contact resistance       111
 *
 * @param data device context (module_t)
 * @param x  Array of 2 for x coordinates
 * @param y  Array of 2 for y coordinates
 * @param r  Array of 2 for resisiance
 */
void read_conversion(void *data, uint16_t x[2], uint16_t y[2],
		uint16_t r[2]) {
	input_module_t *module = (input_module_t *) data;
	private_data_t *dp = module->data;
	uint32_t buf = 0, raw_data = 0;
	uint16_t raw_x[4];
	uint16_t raw_y[4];
	uint16_t raw_r[4];
	TRACE;

	/* Read x twice */
	raw_data = i2c_read(dp->hw_ctx, ADC2_REG);
	raw_x[0] = CSP_BITFEXT (raw_data, MC13892_ADC2_ADD1);
	raw_x[1] = CSP_BITFEXT (raw_data, MC13892_ADC2_ADD2);

	raw_data = i2c_read(dp->hw_ctx, ADC2_REG);
	raw_x[2] = CSP_BITFEXT (raw_data, MC13892_ADC2_ADD1);
	raw_x[3] = CSP_BITFEXT (raw_data, MC13892_ADC2_ADD2);

	/* dummy read */
	raw_data = i2c_read(dp->hw_ctx, ADC2_REG);

	/* Read y twice */
	raw_data = i2c_read(dp->hw_ctx, ADC2_REG);
	raw_y[0] = CSP_BITFEXT (raw_data, MC13892_ADC2_ADD1);
	raw_y[1] = CSP_BITFEXT (raw_data, MC13892_ADC2_ADD2);
	raw_data = i2c_read(dp->hw_ctx, ADC2_REG);
	raw_y[2] = CSP_BITFEXT (raw_data, MC13892_ADC2_ADD1);
	raw_y[3] = CSP_BITFEXT (raw_data, MC13892_ADC2_ADD2);

	/* dummy read */
	raw_data = i2c_read(dp->hw_ctx, ADC2_REG);

	/* Contact resistance twice */
	raw_data = i2c_read(dp->hw_ctx, ADC2_REG);
	raw_r[0] = CSP_BITFEXT (raw_data, MC13892_ADC2_ADD1);
	raw_r[1] = CSP_BITFEXT (raw_data, MC13892_ADC2_ADD2);
	raw_data = i2c_read(dp->hw_ctx, ADC2_REG);
	raw_r[2] = CSP_BITFEXT (raw_data, MC13892_ADC2_ADD1);
	raw_r[3] = CSP_BITFEXT (raw_data, MC13892_ADC2_ADD2);

	/*
	 * Disable the ADC
	 * Reg ADC1
	 * bit 0 ADEN - 0 Disable ADC
	 */
	buf = i2c_read(dp->hw_ctx, ADC1_REG);
	buf &= ~(1 << 0);
	buf |= (0 << 0);
	i2c_write(dp->hw_ctx, ADC1_REG, buf);

	DEBUG_CMD(slogf(99,1,"read_conversion x:%d x:%d x:%d x:%d y:%d y:%d y:%d y:%d r:%d r:%d r:%d r:%d\n", raw_x[0], raw_x[1], raw_x[2], raw_x[3], raw_y[0], raw_y[1], raw_y[2], raw_y[3], raw_r[0], raw_r[1], raw_r[2], raw_r[3]);)
	/* average values between ADC channels in the same sample, values mostly equal so this might not be needed */
	x[0] =(raw_x[0] + raw_x[1])/2;
	x[1] =(raw_x[2] + raw_x[3])/2;
	y[0] =(raw_y[0] + raw_y[1])/2;
	y[1] =(raw_y[2] + raw_y[3])/2;
	r[0] =(raw_r[0] + raw_r[1])/2;
	r[1] =(raw_r[2] + raw_r[3])/2;
	DEBUG_CMD(slogf(99,1,"read_conversion x:%d x:%d y:%d y:%d r:%d r:%d\n", x[0], x[1], y[0], y[1], r[0], r[1]);)

	if (dp->verbose >= 1) {
		printf("read_conversion x:%d x:%d y:%d y:%d r:%d r:%d\n", raw_x[0],
				raw_x[1], raw_y[0], raw_y[1], raw_r[0], raw_r[1]);
	}

}

/**
 * Interrupt handler function.
 * This code is run by the interrupt handler thread.  It waits on a pulse
 * generated by the interrupt and then requests the X and Y coordinates
 * from the touch controller (MC13892).
 * @param data device context (module_t)
 * @note Once the data has been fetched, a timer is started to poll for a
 * more points or a release event. 
 * 
 */
static void *
intr_thread(void *data) {
	input_module_t *module = (input_module_t *) data;
	private_data_t *dp = module->data;
	input_module_t *up = module->up;
	struct _pulse pulse;
	iov_t iov;
	int rcvid;
	uint16_t raw_x[2], raw_y[2], raw_r[2];
	int x, y;
	TRACE;

	SETIOV(&iov, &pulse, sizeof(pulse));

	while (1) {
		if ((rcvid = MsgReceivev(dp->chid, &iov, 1, NULL)) == -1) {
			if (errno == ESRCH) {
				pthread_exit(NULL);
			}
			continue;
		}

		switch (pulse.code) {
		case PULSE_CODE:
			pthread_mutex_lock(&dp->mutex);
			DEBUG_CMD(slogf(99,1,"Got interrupt IRQ status reg %x", i2c_read(dp->hw_ctx, INT_STATUS_0_REG));)
			if (dp->verbose >= 1) {
				printf("Got Interrupt\n");
			}

			/* Stop timer */
			dp->itime.it_value.tv_sec = 0;
			dp->itime.it_value.tv_nsec = 0;
			dp->itime.it_interval.tv_sec = 0;
			dp->itime.it_interval.tv_nsec = 0;

			/* Set touch release timer */
			timer_settime(dp->timerid, 0, &dp->itime, NULL);

			/* Clear interrupt and Unmask */
			i2c_write(dp->hw_ctx, INT_STATUS_0_REG,
					MC13892_TSI_BIT);

			/* Put touchscreen into Position Mode */
			set_touchscreen_mode(data, POSITION_MODE);

			/* Start the conversion */
			start_conversion(data);

			/* Read the data from the controller */
			read_conversion(data, raw_x, raw_y, raw_r);

			/* Process Data */
			process_data(raw_x, raw_y, raw_r, &x, &y, data);

			dp->tp.x = x;
			dp->tp.y = y;

			if (dp->verbose >= 1) {
				fprintf(stderr, "X:%d Y:%d State: %s\n", dp->tp.x, dp->tp.y,
						(dp->tp.buttons == 0L) ? "Released" : "Touched");
			}

			/* Emit the data to the upper layers */
			clk_get(&dp->tp.timestamp);
			(up->input)(up, 1, &dp->tp);

			dp->lastx = dp->tp.x;
			dp->lasty = dp->tp.y;

			if (dp->tp.buttons != 0L) {
				/* start the hardware poll timer */
				dp->itime.it_value.tv_sec = 0;
				dp->itime.it_value.tv_nsec = dp->poll_delay;
				dp->itime.it_interval.tv_sec = 0;
				dp->itime.it_interval.tv_nsec = 0;
				timer_settime(dp->timerid, 0, &dp->itime, NULL);
			}

			/* clear status, just in case */
			i2c_write(dp->hw_ctx, INT_STATUS_0_REG, MC13892_TSI_BIT | MC13892_ADCDONE_BIT);

			/* slow interrupts */
			delay (dp->intr_delay);

			set_touchscreen_mode(module, INTERRUPT_MODE);
			pthread_mutex_unlock(&dp->mutex);
			InterruptUnmask(dp->irq, dp->iid);

			break;
		default:
			if (rcvid) {
				MsgReplyv(rcvid, ENOTSUP, &iov, 1);
			}
			break;
		}
	}
}

/**
 * Process the coordinate data.
 * - check for pen up event i.e. resistance below pen threshold
 * - Compare coordinate reject those with too great a spread
 * - Use Hysteresis to reject invalid data.
 * @param data device context (module_t)
 * @param raw_x  Array of 2 for x coordinates
 * @param raw_y  Array of 2 for y coordinates
 * @param raw_r  Array of 2 for resistance
 * @param x  Pointer to x coordinate for processed coordinate
 * @param y  Pointer to y coordinate for processed coordinate
 * @note This code is called from the interrupt handler thread.
 * @return 0-success 1-release event sent -1 - invalid date ignore
 * @note Uses Hysteres analasys to validate data.
 * According to Wikipedia: In a system with hysteresis, this is not possible; there is no way to predict
 * the output without knowing the system's current state, and there is no way to know the
 * system's state without looking at the history of the input. This means that it is necessary
 * to know the path that the input followed before it reached its current value.
 */
int process_data(uint16_t raw_x[2], uint16_t raw_y[2], uint16_t raw_r[2],
		int *x, int *y, void *data) {
	input_module_t *module = (input_module_t *) data;
	private_data_t *dp = module->data;
	int i = 0;
	uint16_t delta;
	static uint16_t hys_x[2];
	static uint16_t hys_y[2];
	static uint32_t hysIndex;
	TRACE;

	// Check for pen up condition
	for (; i < 2; i++) {
		  if (raw_r[i] > dp->touch_threshold) {
			// Got a release
			DEBUG_MSG("resistance below threshold");

			*x = dp->lastx;
			*y = dp->lasty;
			dp->tp.buttons = 0L;

			if (dp->verbose > 2) {
				fprintf(stderr,
						"resistance below threshold injecting a Release.\n");
			}
			return (1);
		}
	}

	// Calculate absolute differences between x-coordinate samples
	delta = ABS (raw_x[0] - raw_x[1]);

	// Reject the samples if the spread is too large
	if ((delta > DELTA_X_COORD_VARIANCE)) {
		// Data is invalid
		if (dp->verbose > 2) {
			fprintf(stderr, "Data is invalid X Spread is too large.\n");
		}
		return (-1);
	}

	*x = raw_x[0] + raw_x[1];

	// Calculate absolute differences between y-coordinate samples
	delta = ABS (raw_y[0] - raw_y[1]);

	if ((delta > DELTA_Y_COORD_VARIANCE)) {
		// Data is invalid
		if (dp->verbose > 2) {
			fprintf(stderr, "Data is invalid Y spread is too large.\n");
		}
		return (-1);
	}
	*y = raw_y[0] + raw_y[1];

	if (!dp->tp.buttons) {
		// Prime the hysteresis buffers with average of two
		// best samples from ADC
		*x = *x >> 1;
		*y = *y >> 1;
		hys_x[0] = hys_x[1] = *x;
		hys_y[0] = hys_y[1] = *y;
	} else {
		// Implement noise rejection since transition to pen up
		// condition often gives us a spike in samples
		if (ABS (*x - (hys_x[0] + hys_x[1])) > (DELTA_X_COORD_VARIANCE * 8)) {
			// Data is invalid
			if (dp->verbose > 2) {
				fprintf(stderr,
						"Data is invalid X Hystersis data is too great variance.\n");
			}

			return (-1);
		}

		if (ABS (*y - (hys_y[0] + hys_y[1])) > (DELTA_Y_COORD_VARIANCE * 8)) {
			// Data is invalid
			if (dp->verbose > 2) {
				fprintf(stderr,
						"Data is invalid Y Hystersis data is too great variance.\n");
			}
			return (-1);
		}

		// Average two best samples from ADC with samples
		// from hysteresis buffer
		*x = (hys_x[0] + hys_x[1] + *x) >> 2;
		*y = (hys_y[0] + hys_y[1] + *y) >> 2;

		// Replace an entry in hysteresis buffer
		hys_x[hysIndex & 0x1] = *x;
		hys_y[hysIndex & 0x1] = *y;

		hysIndex++;
	}

	dp->tp.buttons = _POINTER_BUTTON_LEFT;

	return (0);
}

/**
 *
 * Timer pulse handler gets called when the poll timer has expired.
 * Put MC13892 in Possition mode, reads, process, and injects coordinate
 * data into input runtime.
 */
static int touch_pulse(message_context_t * ctp, int code, unsigned flags,
		void *data) {
	input_module_t *module = (input_module_t *) data;
	input_module_t *up = module->up;
	private_data_t *dp = module->data;
	uint16_t raw_x[2], raw_y[2], raw_r[2];
	int x, y;
	uint32_t buf = 0;
	TRACE;

	InterruptMask(dp->irq, dp->iid);
	pthread_mutex_lock(&dp->mutex);

	/* Put touchscreen into Position Mode */
	set_touchscreen_mode(data, POSITION_MODE);

	/* clear status */
	i2c_write(dp->hw_ctx, INT_STATUS_0_REG, MC13892_TSI_BIT | MC13892_ADCDONE_BIT);

	/* Start the conversion */
	start_conversion(data);

	/* Read the data from the controller */
	read_conversion(data, raw_x, raw_y, raw_r);

	/* Process Data */
	process_data(raw_x, raw_y, raw_r, &x, &y, data); 

	dp->tp.x = x;
	dp->tp.y = y;

	if (dp->verbose >= 1) {
		fprintf(stderr, "X:%d Y:%d State: %s\n", dp->tp.x, dp->tp.y,
				(dp->tp.buttons == 0L) ? "Released" : "Touched");
	}

	dp->lastx = dp->tp.x;
	dp->lasty = dp->tp.y;

	/* Emit the data to the upper layers */
	clk_get(&dp->tp.timestamp);
	(up->input)(up, 1, &dp->tp);

	if (dp->tp.buttons != 0L) {
		/* restart the hardware poll timer */
		dp->itime.it_value.tv_sec = 0;
		dp->itime.it_value.tv_nsec = dp->poll_delay;
		dp->itime.it_interval.tv_sec = 0;
		dp->itime.it_interval.tv_nsec = 0;

		/* Set touch release timer */
		timer_settime(dp->timerid, 0, &dp->itime, NULL);
	}
	/* clear status, just in case */
	i2c_write(dp->hw_ctx, INT_STATUS_0_REG, MC13892_TSI_BIT | MC13892_ADCDONE_BIT);
	/* re-enable touchscreen interrupt */
	buf = i2c_read(dp->hw_ctx, INT_MASK_0_REG);
	buf &= ~MC13892_TSI_BIT;
	i2c_write(dp->hw_ctx, INT_MASK_REG, buf);
	set_touchscreen_mode (module, INTERRUPT_MODE);
	pthread_mutex_unlock(&dp->mutex);
	InterruptUnmask(dp->irq, dp->iid);

	return (0);
}

/**
 * I2C read register.
 * Read MC13892 register over I2C.
 * @param i2c_fd I2C file descriptor
 * @param reg Register number to read
 * @return Value read from register
 */
static int i2c_read(uint32_t i2c_fd, int reg) {
	struct {
		i2c_sendrecv_t hdr;
		unsigned char bytes[3];
	} msg;
	int status;
	int bytes;
	int ret;
	//TRACE;

	msg.hdr.slave.addr = MC13892_I2C_ADDR;
	msg.hdr.slave.fmt = I2C_ADDRFMT_7BIT;
	msg.hdr.send_len = 1;
	msg.hdr.recv_len = 3;
	msg.hdr.stop = 1;
	msg.bytes[0] = (reg & 0x3f);
	if (status = devctl(i2c_fd, DCMD_I2C_SENDRECV, &msg, sizeof(msg), &bytes)) {
		errno = status;
		perror("SENDRECV");

	}
	ret = msg.bytes[0] << 16 | (msg.bytes[1] << 8) | (msg.bytes[2]);
	DEBUG_CMD (printf ("read reg %d val %x\n", reg, ret));
	return ret;
}

/**
 * I2C write register.
 * Read MC13892 register over I2C.
 * @param i2c_fd I2C file descriptor
 * @param reg Register number to write
 * @param val Value to write
 */
static void i2c_write(uint32_t i2c_fd, int reg, int val) {
	int status;
	int bytes;
	struct {
		i2c_send_t hdr;
		unsigned char bytes[4];
	} msg;
	//TRACE;
	DEBUG_CMD (printf ("write reg %d val %x\n", reg, val));

	msg.hdr.slave.addr = MC13892_I2C_ADDR;
	msg.hdr.slave.fmt = I2C_ADDRFMT_7BIT;
	msg.hdr.len = sizeof(msg.bytes);
	msg.hdr.stop = 1;
	msg.bytes[0] = (reg & 0x3F);
	msg.bytes[1] = ((val >> 16) & 0xFF);
	msg.bytes[2] = ((val >> 8) & 0xFF);
	msg.bytes[3] = ((val >> 0) & 0xFF);

	if (status = devctl(i2c_fd, DCMD_I2C_SEND, &msg, sizeof(msg), &bytes)) {
		errno = status;
		perror("SEND");
	}
}

/**
 * Shutdown touchscreen driver.
 */
static int touch_shutdown(input_module_t * module, int delay) {
	private_data_t *dp = module->data;
	TRACE;

	close(dp->hw_ctx);
	free(module->data);

	return (0);
}

__SRCVERSION ("$URL$ $Rev$")
