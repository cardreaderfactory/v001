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


#define SYSTICK_INTERRUPT_VECTOR_NUMBER 15u /* Cortex-M0 hard vector */ 
#define INTERRUPT_FREQ 1000u  

#include "project.h"
#include "common.h"
#include "debug.h"
#include "xprintf.h"
#include "profiler.h"


volatile uint32_t timestamp = 0;
static volatile uint8_t wdtCounter = 0;
v001stat_t stats;
v001conf_t config;

uint32_t timerUnits(void)
{
    uint32_t ms = CySysWdtGetCount(0);
    uint32_t s = timestamp;
    uint32_t ms1 = CySysWdtGetCount(0);
    if (ms1 < ms)    
        return (timestamp << 15) + ms1; // timer overflow, read again the timestamp.
    else
        return (s << 15) + ms;
        
}

uint32_t GetTick(void)
{
    return timestamp * 1024 + CySysWdtGetCount(0)/(32768/1024);
}

void wdtReset(void)
{
    wdtCounter = 0;    
}

bool TimerExpired(uint32_t ms)
{
    uint32_t tick = GetTick();
    if (tick >= ms && tick - ms < HALF_RANGE(sizeof(ms)))
        return true;
    return false;
}

CY_ISR(ClockTick)
{
    uint32 intSource = CySysWdtGetInterruptSource() ;
    if(intSource & CY_SYS_WDT_COUNTER1_INT)
    {
        Notification_LED_Write(LED_OFF);                    
    	CySysWdtClearInterrupt(CY_SYS_WDT_COUNTER1_INT);        
    }
    
    if(intSource & CY_SYS_WDT_COUNTER0_INT)
    {   
        Notification_LED_Write(LED_ON);     
        if (wdtCounter > 1)
        {
            ClockTick_ISR_ClearPending();
            profiler_showReport();
            vLog(xBUG, "Watchdog was not fed! Rebooting!\n");
            //TODO: save the timestamp and load it at boot!
            
            /* stop the ISR response for following interrupt */
            ClockTick_ISR_Stop();
            
            // will reset in 3 seconds because the watchdog interrupt is configured as CY_SYS_WDT_MODE_INT_RESET
        }
        else
        {   
            timestamp++;
            wdtCounter++;    
        	CySysWdtClearInterrupt(CY_SYS_WDT_COUNTER0_INT);
//            if (miliseconds != 1023 && miliseconds > 1000)
//                vLog(xDebug,"count: %lu, ms = %lu\n", CySysWdtGetCount(0), miliseconds);
//            miliseconds = CySysWdtGetCount(0)/(32768/1024);
            //ClockTick_ISR_ClearPending();
//            CySysWdtResetCounters(CY_SYS_WDT_COUNTER1_RESET);
//            CySysWdtClearInterrupt(CY_SYS_WDT_COUNTER1_INT);
        }
    }
        
}

CY_ISR(SysTick_ISR) 
{   
//    uint32_t ms = GetTick();
//    if (ms >= 0 && ms <= 20)
//        Notification_LED_Write(LED_ON);
//    else if (miliseconds == 20)
//        Notification_LED_Write(LED_OFF);
}

void startSysTick()
{
    CyIntSetSysVector(SYSTICK_INTERRUPT_VECTOR_NUMBER, SysTick_ISR);  /* Point the Systick vector to the ISR */ 
    SysTick_Config(CYDEV_BCLK__HFCLK__HZ / INTERRUPT_FREQ);      /* Set the number of ticks between interrupts */     
    
}

#if !defined (get16bits)
#define get16bits(d) ((((uint32_t)(((const uint8_t *)(d))[1])) << 8)+(uint32_t)(((const uint8_t *)(d))[0]) )
#endif

uint32_t SuperFastHash (const void * p, int len) 
{
    const char * data = p;
    uint32_t hash = len, tmp;
    int rem;

    if (len <= 0 || data == NULL) return 0;

    rem = len & 3;
    len >>= 2;

    /* Main loop */
    for (;len > 0; len--) {
        hash  += get16bits (data);
        tmp    = (get16bits (data+2) << 11) ^ hash;
        hash   = (hash << 16) ^ tmp;
        data  += 2*sizeof (uint16_t);
        hash  += hash >> 11;
    }

    /* Handle end cases */
    switch (rem) {
        case 3: hash += get16bits (data);
                hash ^= hash << 16;
                hash ^= ((signed char)data[sizeof (uint16_t)]) << 18;
                hash += hash >> 11;
                break;
        case 2: hash += get16bits (data);
                hash ^= hash << 11;
                hash += hash >> 17;
                break;
        case 1: hash += (signed char)*data;
                hash ^= hash << 10;
                hash += hash >> 1;
    }

    /* Force "avalanching" of final 127 bits */
    hash ^= hash << 3;
    hash += hash >> 5;
    hash ^= hash << 4;
    hash += hash >> 17;
    hash ^= hash << 25;
    hash += hash >> 6;

    return hash & 0x7fffffff;
}


uint8_t count_bits(uint32_t v)
{
    v = v - ((v >> 1) & 0x55555555);                        // reuse input as temporary
    v = (v & 0x33333333) + ((v >> 2) & 0x33333333);         // temp
    return ((((v + (v >> 4)) & 0xF0F0F0F) * 0x1010101) >> 24); // count
}

char *maptobin(uint32_t binary)
{
    static char st[32+1];
    char *s = st;
    for (int i = 0; i < 32; i++)
    {
        if (binary & 1UL<<i)
            *s='1';
        else
            *s='0';

        s++;
    }
    *s=0;
    return st;
}


void print_mac(uint32_t flags, CYBLE_GAP_BD_ADDR_T *device, uint16_t serviceUuid)
{
    if (((1<<flags) & vLogFlags) == 0)
        return;
    
    xprintf("mac: %02x%02x%02x%02x%02x%02x, service: %04x\n",
                device->bdAddr[5],
                device->bdAddr[4],
                device->bdAddr[3],
                device->bdAddr[2],
                device->bdAddr[1],
                device->bdAddr[0],
                serviceUuid);
    
    // advReport->rssi -> signal
    // advReport->eventType -> directed advertising, scan response
    // advReport->peerAddrType -> public/random
    
}

void print_auth_info(uint32_t flags, CYBLE_GAP_AUTH_INFO_T *info, char *s)
{    
    if (((1<<flags) & vLogFlags) == 0)
        return;
    if (info != NULL)
        vLog(xBle, "%s authinfo: security=0x%x, bonding=0x%x, ekeySize=0x%x, err=0x%x \n",
                s,
                info->security,
                info->bonding,
                info->ekeySize,
                info->authErr);
    else
        vLog(xBle, "%s authinfo: NULL\n",
                s);
        
}


uint32_t mask(uint8_t len)
{
    switch (len)
    {
    case 0:
        assert(0);
        return 0;
    case 1:
        return 0xff;
    case 2:
        return 0xffff;
    case 3:
        return 0xffffff;
    default:
        assert(0);
        break;            
    }
    
    return 0xffffffff;
}

uint32_t crc32_sft(uint32_t data, uint32_t crc)
{
  static const uint32_t CrcTable[16] = { // Nibble lookup table for 0x04C11DB7 polynomial
    0x00000000,0x04C11DB7,0x09823B6E,0x0D4326D9,0x130476DC,0x17C56B6B,0x1A864DB2,0x1E475005,
    0x2608EDB8,0x22C9F00F,0x2F8AD6D6,0x2B4BCB61,0x350C9B64,0x31CD86D3,0x3C8EA00A,0x384FBDBD };

  crc = crc ^ data; // Apply all 32-bits

  // Process 32-bits, 4 at a time, or 8 rounds

  crc = (crc << 4) ^ CrcTable[crc >> 28]; // Assumes 32-bit reg, masking index to 4-bits
  crc = (crc << 4) ^ CrcTable[crc >> 28]; //  0x04C11DB7 Polynomial used in STM32
  crc = (crc << 4) ^ CrcTable[crc >> 28];
  crc = (crc << 4) ^ CrcTable[crc >> 28];
  crc = (crc << 4) ^ CrcTable[crc >> 28];
  crc = (crc << 4) ^ CrcTable[crc >> 28];
  crc = (crc << 4) ^ CrcTable[crc >> 28];
  crc = (crc << 4) ^ CrcTable[crc >> 28];

  return(crc);
}


/**
 * returns the crc32 of the specified buffer using the 0x4C11DB7 polynomial
 * the crc is done by software
 *
 * buf      pointer to the buffer to run CRC on
 * len      length of buffer (in bytes)
 * crc      old crc value. use 0xffffffff to start a new CRC
 */
uint32_t crc32(const void *buf, int len, uint32_t crc)    
{    
    int              len32;
    const uint32_t  *b;
    uint32_t v;

    b = buf;
    len32 = len / 4;
    len = len % 4;

    for (int i = 0; i < len32; i++)
    {
        memcpy(&v, &b[i], 4);
        crc = crc32_sft(v, crc);
    }
    
    if (len != 0)
    {
        v = 0;
        memcpy(&v, &b[len32], len);
        
        crc = crc32_sft(v & mask(len), crc);
    }
    
    return crc;    
}


void showStats(uint32_t flags, v001stat_t *stats)
{
    if (((1<<flags) & vLogFlags) == 0)
        return;

    stats->timestamp = timestamp;
    char * isFullText;
    if (isFull(stats)) isFullText = "full"; else isFullText = "demo";
    xprintf("name=" DEVICE_NAME "_%s,build=%06z,total=%lu,used=%lu,free=%lu,current=%lu,all=%lu,time=%lu,fw=" VERSION ",hw=%u,keys=%lu,lkeys=%lu,temp=%i\n",
           isFullText,
           stats->serialNumber,
           stats->memorySize,
           stats->memoryUsed,
           stats->memorySize - stats->memoryUsed,
           stats->swipeCount,
           stats->lifeSwipeCount,
           stats->timestamp,
           stats->hwFlags,
           stats->keysCount,
           stats->lifeKeysCount,
           stats->dieTemperature);

}

uint32_t ls = 0;    

void print_minute_mark(void)
{
    if (timestamp >= ls + 60)
    {
        ls = timestamp;
        vLog(xInfo, "-- minute mark --\n");
    }
}    

char * parseHandleString(char *st)
{
    static char s[50];
    int len = strlen(st);
    bool first = true;
    uint16_t i;
    for (i = 0; i < len && i < sizeof(s) - 1; i++)
    {
        s[i] = toupper((int)st[i]);
        if (first)
        {
            first = false;
            continue;
        }
        if (st[i] == '_')
        {
            s[i] = ' ';
            first = true;
            continue;
        }
        s[i] = tolower((int)st[i]);        
    }
    s[i] = 0;
//    len = strlen(s) - 1;
//    while(len > 0 && s[len] != 0)
//        len--;
//    
//    s[len] = 0;
    return s;
}

char *cmd2st(int t)
{
    switch (t)
    {        
        C2ST(SERVICE                       )
        C2ST(DEVICE_STATUS_DECL            )
        C2ST(DEVICE_STATUS_CHAR            )
        C2ST(DEVICE_STATUS_CCCD_DESC       )
        C2ST(DOWNLOAD_DECL                 )
        C2ST(DOWNLOAD_CHAR                 )
        C2ST(DOWNLOAD_CCCD_DESC            )
        C2ST(MANUFACTURER_NAME_STRING_DECL )
        C2ST(MANUFACTURER_NAME_STRING_CHAR )
        C2ST(MODEL_NUMBER_STRING_DECL      )
        C2ST(MODEL_NUMBER_STRING_CHAR      )
        C2ST(SERIAL_NUMBER_STRING_DECL     )
        C2ST(SERIAL_NUMBER_STRING_CHAR     )
        C2ST(HARDWARE_REVISION_STRING_DECL )
        C2ST(HARDWARE_REVISION_STRING_CHAR )
        C2ST(FIRMWARE_REVISION_STRING_DECL )
        C2ST(FIRMWARE_REVISION_STRING_CHAR )
        C2ST(LOGIN_DECL                    )
        C2ST(LOGIN_CHAR                    )
        C2ST(SETKEY_DECL                   )
        C2ST(SETKEY_CHAR                   )
        C2ST(SETNAME_DECL                  )
        C2ST(SETNAME_CHAR                  )
        C2ST(SETTIME_DECL                  )
        C2ST(SETTIME_CHAR                  )
        C2ST(ERASE_DECL                    )
        C2ST(ERASE_CHAR                    )
        C2ST(SETPASS_DECL                  )
        C2ST(SETPASS_CHAR                  )
        C2ST(UPGRADE_DECL                  )
        C2ST(UPGRADE_CHAR                  )
        C2ST(LOGFLAGS_DECL                 )
        C2ST(LOGFLAGS_CHAR                 )
        C2ST(REBOOT_DECL                   )
        C2ST(REBOOT_CHAR                   )
    }        
    return "???";
}


