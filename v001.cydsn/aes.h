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

#ifndef AES_H
#define AES_H

#include "stdint.h"
#include "stdbool.h"

#define KEYBITS 128                                 //!< Use AES128.
    
void aesInit(const uint8_t *key);
void aesDecrypt(uint8_t *buffer, uint8_t *chainBlock);
void flip(uint8_t *v);

#endif // AES_H

