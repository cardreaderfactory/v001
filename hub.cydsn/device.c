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


void print_boot_complete(void)
{
    xprintf("_________________ __________       ______  _______  __________\n");
    xprintf("__  ____/___  __ \\___  ____/       ___  / / /__  / / /___  __ )\n");
    xprintf("_  /     __  /_/ /__  /_           __  /_/ / _  / / / __  __  |\n");
    xprintf("/ /___   _  _, _/ _  __/           _  __  /  / /_/ /  _  /_/ /\n");
    xprintf("\\____/   /_/ |_|  /_/              /_/ /_/   \\____/   /_____/\n");
    xprintf("\n" DEVICE_NAME " " COPYRIGHT " v" SOFTWARE_VERSION " (" BUILD_TYPE ") build on " __DATE__ " " __TIME__ "\n\n");

#ifdef DEBUG
    vLogFlags = ALL_LOGS_ON;
#else
    vLogFlags = RELEASE_LOGS_ON;
#endif
//    printf("Press '?' to display the menu\n\n");
}