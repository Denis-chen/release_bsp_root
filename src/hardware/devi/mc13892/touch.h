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
 */


#include <errno.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/neutrino.h>
#include <sys/time.h>

//#define TRACE_DEBUG
#if defined(TRACE_DEBUG)
#define TRACE  slogf(99,1, "TRACE [%s]", __FUNCTION__)
#define TRACE_ENTER slogf(99,1, "%s enter", __FUNCTION__)
#define TRACE_EXIT slogf(99,1, "%s exit", __FUNCTION__)
#define DEBUG_MSG(x) slogf(99,1, "%s %s", __FUNCTION__, x)
#define DEBUG_CMD(x)  x
#else
/** Trace function execution.
 * @note Compile time switched debug 
 */
#define TRACE
/** Trace function enter execution.
 * @note Compile time switched debug 
 */
#define TRACE_ENTER
/** Trace function exit execution.
 * @note Compile time switched debug 
 */
#define TRACE_EXIT 
/** Debug message.
 * @note Compile time switched debug 
 */
#define DEBUG_MSG(x) 
/** Debug command.
 * @note Compile time switched debug 
 */
#define DEBUG_CMD(x)  
#endif

/** Touch sreen interrupt */
#define TOUCH_INT         96

/** 
 * Touch sreen inactive mode 
 * MC13892 is neither waiting for a touch event or calculatinig a position
 * @note May never enter into this mode
 */
#define INACTIVE_MODE     0
/**
 * Touch screen interrupt mode 
 * MC13892 is waiting for a touch event.  In this mode an interrupt is create when the 
 * touch screen planes touch, i.e. when the screen is touched 
 */ 
#define INTERRUPT_MODE    1
/** 
 * Touch screen position mode.  
 * MC13892 is calculating the position of a touch events.  
 */ 
#define POSITION_MODE     2


/** Maximum allowed variance in the X coordinate samples. */
#define DELTA_X_COORD_VARIANCE          24

/** Maximum allowed variance in the X coordinate samples. */
#define DELTA_Y_COORD_VARIANCE          24

#define ABS(x)  ((x) >= 0 ? (x) : (-(x)))

/** Driver state initialize  */
#define FLAG_INIT         0x1000
/** Driver state reset  */
#define FLAG_RESET        0x2000

/** Default minimum time between interrupt */
#define INTR_DELAY        75
/** Default time between hardware polls, this is default the rate of event injection */
#define HW_POLL_TIME     100

/** @brief Interrupt plus priority */
#define PULSE_PRIORITY    21
/** @brief Interrupt pulse code */
#define PULSE_CODE        1
/**  Default I2C address device name */
#define MC13892_I2C_DEVICE "/dev/i2c0"
/** I2C address of MC13892 */
#define MC13892_I2C_ADDR 8
/** I2C Default bus speed */
#define MC13892_I2C_BUS_SPEED  100000

/**
 * Touch screen minimum resistance default.
 * This values is used to denote the end of a touch event.
 */
#define MC13892_TOUCH_RESISTANCE_DEFAULT 800



/*
 *      TSMOD2 TSMOD1 TSMOD0 Mode      Description
 *      x      0      0   Inactive  Inputs TSX1, TSX2, TSY1, TSY2 can be used as general 
 *                        purpose ADC inputs
 *      0      0      1   Interrupt Interrupt detection is active. Generates an interrupt 
 *                        TSI when plates make contact.  TSI is dual edge sensitive and 30ms debounced
 *      1      0      1   Reserved  Reserved for a different interrupt mode
 *      0      1      x   Touch     ADC will control a sequential reading of 2 times a XY coordinate
 *                        Screen    pair and 2 times a contact resistance
 *      1      1      x   Reserved  Reserved for a different reading mode
 * 
 * 
 *  Bit 15 CHRGRAWDIV   Sets CHRGRAW scaling to divide by 5
 */
/** Mask for MS13892 Mode */
#define MC13892_MODE_MASK        (1<<15|1<<14|1<<13|1<<12)
/** Interrupt mode. Used for tocuh screen INTERRUPT_MODE */
#define MC13892_INTERRUPT_MODE   (1<<15|0<<14|0<<13|1<<12)
/** Touch sceen mode. Used for positional mode, ADC read x,y, restance   */
#define MC13892_TOUCHSCREEN_MODE (1<<15| 0<<14|1<<13)
/** Inactive sceen mode.*/ 
#define MC13892_INACTIVE_MODE    (0<<13|0<<12)

/**
 * MC13892 Interrupt status register 0 
 */
#define INT_STATUS_0_REG           0x0
/**
 * MC13892 Interrupt mask register 0 
 */
#define INT_MASK_0_REG           0x1
/**  bit 0 ADCDONEI    ADC has finished requested conversions */
#define  MC13892_ADCDONE_BIT (1<<0)
 /**  bit 2 TSI         Touch screen wakeup */
#define  MC13892_TSI_BIT     (1<<2)


/** MC13892 offset ADC register 0 
 * @note Refer to MC13892 Users Guide 034.pdf for more information 
 */
#define ADC0_REG          43
/** MC13892 offset ADC register 1 
 * @note Refer to MC13892 Users Guide 034.pdf for more information 
 */
#define ADC1_REG          44
/** MC13892 offset ADC register 2 
 * @note Refer to MC13892 Users Guide 034.pdf for more information 
 */
#define ADC2_REG          45 
/** MC13892 ADC conversion done bit. 
 * @note Refer to MC13892 Users Guide 034.pdf for more information 
 */
#define ADCDONE1          0x000001
/** MC13892 ADC conversion done bit mask. 
 * @note Refer to MC13892 Users Guide 034.pdf for more information 
 */
#define ADCDONE1_MASK     0xFFFFFE
/** MC13892 interrupt mask register 0
 * @note Refer to MC13892 Users Guide 034.pdf for more information 
 */
#define INT_MASK_REG      1
/** MC13892 Switcher 4 register
 * For PLL control
 * @note Refer to MC13892 Users Guide 034.pdf for more information 
 */
#define TIMER_REG         28



/**
 *  Default I2C address device name
 */
#define MC13892_I2C_DEVICE "/dev/i2c0"

/**
 * I2C address of MC13892
 */
#define MC13892_I2C_ADDR 8

/**
 * I2C Default bus speed
 */
#define MC13892_I2C_BUS_SPEED  100000


#define PLL_TIMER         0x3C000


/*
 *       TSMOD2 TSMOD1 TSMOD0 Mode      Description
 *      x      0      0   Inactive  Inputs TSX1, TSX2, TSY1, TSY2 can be used as general 
 *                        purpose ADC inputs
 *      0      0      1   Interrupt Interrupt detection is active. Generates an interrupt 
 *                        TSI when plates make contact.  TSI is dual edge sensitive and 30ms debounced
 *      1      0      1   Reserved  Reserved for a different interrupt mode
 *      0      1      x   Touch     ADC will control a sequential reading of 2 times a XY coordinate
 *                        Screen    pair and 2 times a contact resistance
 *      1      1      x   Reserved  Reserved for a different reading mode
 */
#define MC13892_MODE_MASK        (1<<15|1<<14|1<<13|1<<12)
#define MC13892_INTERRUPT_MODE   (1<<15|0<<14|0<<13|1<<12)
#define MC13892_TOUCHSCREEN_MODE (1<<15| 0<<14|1<<13)
#define MC13892_INACTIVE_MODE    (0<<13|0<<12)


/**
 * MC13892 Interrupt status register 0 
 */
#define  MC13892_INTERRUPT_STATUS_REG0  0
/**
 * MC13892 Interrupt mask register 0 
 */
#define  MC13892_INTERRUPT_MASK0        1
/**  bit 0 ADCDONEI    ADC has finished requested conversions */
#define  MC13892_ADCDONE_BIT (1<<0)
 /**  bit 2 TSI         Touch screen wakeup */
#define  MC13892_TSI_BIT     (1<<2)


#define MC13892_ADC2_ADD1_LSH        2
#define MC13892_ADC2_ADD1_WID        10

#define MC13892_ADC2_ADD2_LSH        14
#define MC13892_ADC2_ADD2_WID        10


#define CSP_BITFMASK(bit) (((1U << (bit ## _WID)) - 1) << (bit ## _LSH))
#define CSP_BITFVAL(bit, val) ((val) << (bit ## _LSH))
#define CSP_BITFEXT(var, bit) ((var & CSP_BITFMASK(bit)) >> (bit ## _LSH))




__SRCVERSION( "$URL$ $Rev$" )
