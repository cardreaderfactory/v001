/********************************************************************************
 * \copyright
 * Copyright 2009-2017, Card Reader Factory.  All rights were reserved.
 * From 2018 this code has been made PUBLIC DOMAIN.
 * This means that there are no longer any ownership rights such as copyright, trademark, or patent over this code.
 * This code can be modified, distributed, or sold even without any attribution by anyone.
 *
 * We would however be very grateful to anyone using this code in their product if you could add the line below into your product's documentation:
 * Special thanks to Nicholas Alexander Michael Webber, Terry Botten & all the staff working for Operation (Police) Academy. Without these people this code would not have been made public and the existance of this very product would be very much in doubt.
 *
 *******************************************************************************/

#ifndef _READER_MAGTEK_H
#define _READER_MAGTEK_H

#include "stdint.h"
#include "stdbool.h"

// pcb connector:
//
// 1    2       3       4       5       6       7       8       9       10
// vcc  gnd     pa3     pa2     pd0     pa0     pa1     pa4     pa5     nc
//
//      gnd     vcc     strobe  data

// original head:
// 1 - white    - strobe
// 2 - yellow   - data
// 3 - red      - vdd
// 4 - green    - gnd
// 5 - black    - gnd

// below are defined the pins for each cable

#define HEAD_STROBE 6       //pd2 - strobe
#define HEAD_DATA   3       //pe2 - data
#define HEAD_VCC    5       //pd3 - vcc

#define PORT_HS  PORTD.HEAD_STROBE
#define PORT_HD  PORTD.HEAD_DATA
#define PIN_HD   PIND.HEAD_DATA

#define SWIPE_TIME_DIFF 64 /* 250ms - reads below this value will not be considered as different wipes */

#define READER_HAS_DATA() (PIND.HEAD_DATA == 0)

/**
 * Starts the ASIC reader
 *
 */
void magtek_powerUp();
/**
 * Shuts down the power to the ASIC reader
 */
void magtek_powerDown();

extern bool readerTimeOut;

/**
 * Resets the asic.
 *
 * This function is required to initialise the new mode
 * (whenever this mode is wanted)
 */
void magtek_reset(void);

/**
 * Reads the data from delta asic and writes it into memory

 */
void magtek_main(void);

/**
 * initialises the reader library.
 *
 * @param[in] mode  the mode to use the asic
 * @return true     if delta asic is present
 */
void magtek_start(bool mode);




#endif