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

#include "project.h"
#include "common.h"
#include "debug.h"
#include "xprintf.h"
#include "device.h"

char buf[50];

void print_boot_complete(void)
{
    xprintf("\n______  ___________________         _______ _______ ______\n");
    xprintf("___   |/  /__  ___/___  __ \\___   ____  __ \\__  __ \\__<  /\n");
    xprintf("__  /|_/ / _____ \\ __  /_/ /__ | / /_  / / /_  / / /__  /\n");
    xprintf("_  /  / /  ____/ / _  _, _/ __ |/ / / /_/ / / /_/ / _  /\n");
    xprintf("/_/  /_/   /____/  /_/ |_|  _____/  \\____/  \\____/  /_/\n");
    xprintf("\n" DEVICE_NAME " " COPYRIGHT " v" SOFTWARE_VERSION " (" BUILD_TYPE ") build on " __DATE__ " " __TIME__ "\n\n");

#ifdef DEBUG
    vLogFlags = ALL_LOGS_ON;
#else
    vLogFlags = RELEASE_LOGS_ON;
#endif
//    printf("Press '?' to display the menu\n\n");
}

void upgrade(uint8_t *st, uint8_t len)
{    
    len = min(len, sizeof(buf)-1);
    memcpy(buf, st, len);
    buf[len] = 0;
    vLog(xDebug, "%s(%s): NOT IMPLEMENTED!\n", __FUNCTION__, buf);
}
void setName(uint8_t *st, uint8_t len)
{
    len = min(len, sizeof(buf)-1);
    memcpy(buf, st, len);
    buf[len] = 0;    
    vLog(xDebug, "%s(%s): NOT IMPLEMENTED!\n", __FUNCTION__, buf);
}
void setKey(uint8_t *st, uint8_t len)
{
    len = min(len, sizeof(buf)-1);
    memcpy(buf, st, len);
    buf[len] = 0;    
    vLog(xDebug, "%s(%s): NOT IMPLEMENTED!\n", __FUNCTION__, buf);
}
void setPass(uint8_t *st, uint8_t len)
{
    len = min(len, sizeof(buf)-1);
    memcpy(buf, st, len);
    buf[len] = 0;
    vLog(xDebug, "%s(%s): NOT IMPLEMENTED!\n", __FUNCTION__, buf);
}
void login(uint8_t *st, uint8_t len)
{
    len = min(len, sizeof(buf)-1);
    memcpy(buf, st, len);
    buf[len] = 0;
    vLog(xDebug, "%s(%s): NOT IMPLEMENTED!\n", __FUNCTION__, buf);
}

void setTime(uint8_t *val)
{
    if (val != NULL)
    {
        memcpy((void*)&timestamp, val, 4);            
        vLog(xDebug, "new timestamp: %lu\n", timestamp);
    }
    else
    {
        timestamp = 0;
    }
    CySysWdtResetCounters(CY_SYS_WDT_COUNTER0_RESET);
    while(CySysWdtGetCount(0) < 32768/1024*20); // wait 20 ms - time we want the led to be turned off...
    CySysWdtResetCounters(CY_SYS_WDT_COUNTER1_RESET);
}

