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

#ifndef _MEMCYFLASH_H
#define _MEMCYFLASH_H

#include <stdint.h>
#include <stdbool.h>


    #define MIN_FIRMWARE_SIZE (100000)
    
    bool isMemoryEmpty(void *p);
    void *rowAddr(uint32_t row);
    void *findUsedMemory(void);
    bool flashWrite(const void *p, void *buf, uint16_t len);
    bool flashWriteRow(int row, void *buf);
    void flashWrite_uint32(const void *p, uint32_t value);
    uint32_t flashRead_uint32(const void *p);
    void flashFill(void);
    void flashErase(void);
    void CyFlash_Start(void);
    uint16_t flashFreeBytesInPage(void *p);
    void *flashSp(); // return start pointer
    void *flashSp2(); // different way to read the start pointer 
    void *flashWp(); // returns write pointer
    void *flashEp(); // returns end pointer
    bool flashSetWp(void *p);
    


#endif