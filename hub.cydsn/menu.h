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

#ifndef __MENU_H__
#define __MENU_H__

#include <stdint.h>
#include <stdbool.h>


//bool scan_int(int *val, int minimum, int maximum);
//void print_boot_start(void);
void print_boot_complete(void);
void menu(void);
void check_duplicate_menu_keys(void);

#endif

