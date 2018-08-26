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

#include "readerMagtek.h"

#define AsicBits_NewMode 704 /* cannot enum the damn thing as it is out of range */
#define AsicBits_OldMode 608 /* cannot enum the damn thing as it is out of range */

enum
{
    AsicBufSize_NewMode = AsicBits_NewMode / 8,
    AsicBufSize_OldMode = AsicBits_OldMode / 8,
    AsicFwBits_NewMode = 16,
    AsicFwBits_OldMode = 18
};

// as CVAVR clears global variables at program startup, there's no need to set the below variables to 0 and false
// by not setting them we keep the size of the firmware smaller

char            track_len[3];                       // the length of the data fron track_buf
char            track_buf[3][AsicBufSize_NewMode];  // the card data read from asic
bool            readerTimeOut;
bool            newMode;

 



void writecard(void)
{
    char headerBuf[12];
#ifdef READ_ONE_TRACK
    track_len[0] = 0;
    track_len[2] = 0;
#endif
    /* print header */
    headerBuf[0] = 'S';
    headerBuf[1] = 'C';
    headerBuf[2] = 'R';
    headerBuf[3] = 1;
    memcpy
    *((unsigned long *)(&buf[4])) = timestamp;
    headerBuf[8] = (char)timestamp2();
    headerBuf[9] = track_len[0];
    headerBuf[10] = track_len[1];
    headerBuf[11] = track_len[2];

#ifdef _MEGA328P_INCLUDED_
    if (rtcState == OnT2)
        switchT2toT1();
#endif

    write(buf,12);

    /* print tracks */
    write(track_buf[0], track_len[0]);
    write(track_buf[1], track_len[1]);
    write(track_buf[2], track_len[2]);

//  printf("timestamp = %lu\n", timestamp);
    scheduleCallback_flush(2000);

//  we won't flush here as we will flush it on clock interrupt
//  flush();
    
    
}


void magtek_main(void)
{
    
}

void magtek_start(bool mode)
{
}

/**
 * Starts the ASIC reader
 *
 */
void magtek_powerUp()
{
}
/**
 * Shuts down the power to the ASIC reader
 */
void magtek_powerDown()
{
}

void magtek_reset(void)
{
}
