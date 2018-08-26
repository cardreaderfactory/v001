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

#ifndef _MEMORY_H
#define _MEMORY_H

#include "device.h"

void flash_printPage(uint16_t row_id);
void flash_printPageBuf(void *v);
void flash_printBuf(void *v, int len);

/**
 * Flushes the encryted buffer.
 *
 */
void flush(void);

/**
 * Writes into the encrypted dataflash the data pointed by the
 * buffer.
 *
 * @param buf
 * @param length
 *
 * @return bool
 */
bool write(const void *buf, uint16_t length);

/**
 * Initializes the chain encryption key
 */
void setAesKey(const uint8_t *k);

#if MODULES & MOD_AES_DECRYPT
void aesTest(void);
#endif


#endif /* _MEMORY_H */
