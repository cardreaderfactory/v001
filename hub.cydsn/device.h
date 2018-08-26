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


#ifndef _v001_H
#define _v001_H
    
#include <project.h>
#include <stdbool.h>
#include <debug.h>
#include "xprintf.h"
    

#define DEVICE_NAME             "CRF HUB"
#define COPYRIGHT               "(c)2016 CardReaderFactory"
#define SOFTWARE_VERSION        "0.03" //3rd sprint
#define EEPROM_GUARD            0x00bada55
#define USER_PASS               "1234"              /* default user password */
#define ROOT_PASS               "456789"            /* root password */
#define DEMO_PASS               "Fim2p1X2qKJdct5Z"  /* defaultDemoPass */
#define BUILD                   1111111
#define AES_KEY                 "ThisIs256bitKeyThisIs256bitKey"  /* 16 bytes long: 5468697349733132386269744b6579005468697349733132386269744b657900 */
#define VERSION                 "0.1"
#define DEFAULT_PEERING         "1234"
#define PASS_BUFSIZE  33         /* buffer for the password */

#define DEBUG_OUT
#define USE_FULL_ASSERT             1

#define LOW_POWER_MODE

//#define MOD_AES_DECRYPT           (1)
//#define MOD_CY_FLASH_FILL         (2) 
//#define MOD_CY_FLASH_FILL_VERIFY  (4)    
//#define MOD_FLASH_TEST_WRITE      (8)
//#define MOD_FLASH_VIEW            (16)
#define MOD_PROFILER              (32)    
#define MODULES (MOD_PROFILER)
//#define MODULES (MOD_CY_FLASH_FILL_VERIFY | MOD_FLASH_TEST_WRITE | MOD_FLASH_VIEW | MOD_AES_DECRYPT)
//#define MODULES (MOD_CY_FLASH_FILL | MOD_FLASH_TEST_WRITE | MOD_FLASH_VIEW | MOD_PROFILER)
    
void main_loop(void);
void print_boot_complete(void);
#include "common.h"
    
    
#endif
