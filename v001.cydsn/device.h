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

#include <stdint.h>
#include <stdbool.h>

#define DEVICE_NAME             "MSRv001"
#define COPYRIGHT               "(c)2016 CardReaderFactory"
#define SOFTWARE_VERSION        "0.03" //3rd sprint
    
#define EEPROM_GUARD            0x0bada550
#define USER_PASS               "1234"              /* default user password */
#define ROOT_PASS               "456789"            /* root password */
#define DEMO_PASS               "Fim2p1X2qKJdct5Z"  /* defaultDemoPass */
#define AES_KEY                 "ThisIs128bitKey"  /* 16 bytes long: 5468697349733132386269744b657900 */
#define VERSION                 "0.1"
#define DEFAULT_PEERING         "1234"


#define LOW_POWER_MODE

#define MOD_AES_DECRYPT           (1)
#define MOD_CY_FLASH_FILL         (2) 
#define MOD_CY_FLASH_FILL_VERIFY  (4)    
#define MOD_FLASH_TEST_WRITE      (8)
#define MOD_FLASH_VIEW            (16)    
#define MOD_PROFILER              (32)    
#define MOD_PEAK_DETECT_UNIT_TEST (64)
//#define MODULES (MOD_CY_FLASH_FILL_VERIFY | MOD_FLASH_TEST_WRITE | MOD_FLASH_VIEW | MOD_AES_DECRYPT)
#define MODULES (MOD_CY_FLASH_FILL | MOD_FLASH_TEST_WRITE | MOD_FLASH_VIEW | MOD_PROFILER | MOD_PEAK_DETECT_UNIT_TEST)
    
/* a lot of data */    
#define LOG_FLASH_CACHE             (1) 
#define LOG_CY_FLASH_WRITE          (2) 
#define LOG_AES_DECRYPT             (4)     
#define LOG_FLASH_WRITE_ROW         (8)
/* decent amount of data */    
#define LOG_FLASH_CACHE_INFO        (16)
#define LOG_FLASH_WRITE             (32)
#define LOG_FLASH_DATA_OVERWRITE    (64)
    

#define LOG (LOG_FLASH_DATA_OVERWRITE | LOG_FLASH_WRITE | LOG_FLASH_CACHE_INFO)
    
void upgrade(uint8_t *st, uint8_t len);
void setName(uint8_t *st, uint8_t len);
void setKey(uint8_t *st, uint8_t len);
void setPass(uint8_t *st, uint8_t len);
void login(uint8_t *st, uint8_t len);
void setTime(uint8_t *val);
void print_boot_complete(void);

#endif
