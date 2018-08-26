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


#ifndef _COMMON_H
#define _COMMON_H

#include <stdbool.h>
#include <stdint.h>

//#define PASS_BUFSIZE  33         /* buffer for the password */    
    
#ifdef DEBUG
    #define USE_FULL_ASSERT             1
#endif    

#define AES_BLOCKSIZE (16)

#define ENABLED                     (1u)
#define DISABLED                    (0u)

#define LED_ON                      (0u)
#define LED_OFF                     (1u)

#define __stringnify_1(x...)  #x
#define __stringnify(x...) __stringnify_1(x)
#define E2ST(__A) case __A: return __stringnify(__A);
#define C2ST(__A) case CYBLE_MSRVXXX_ ## __A ## _HANDLE: return parseHandleString(__stringnify(__A));

#ifndef min
#define min(a,b) ((a) > (b) ? (b) : (a))
#endif

#ifndef max
#define max(a,b) ((a) > (b) ? (a) : (b))
#endif

#define tu2ms(VALUE) ( (VALUE) / 32 * 1000 /1024 )  /* convert timer units to miliseconds */


#if  USE_FULL_ASSERT
/**
  * @brief  The assert_param macro is used for function's parameters check.
  * @param  expr: If expr is false, it calls assert_failed function
  *         which reports the name of the source file and the source
  *         line number of the call that failed. 
  *         If expr is true, it returns no value.
  * @retval None
  */
  #define assert(expr) ((expr) ? (void)0 : assert_failed((uint8_t *)__FILE__, __LINE__))
/* Exported functions ------------------------------------------------------- */
  void assert_failed(uint8_t* file, uint32_t line);
#else
  #define assert(expr) ((void)0)  
#endif /* USE_FULL_ASSERT */

#define HALF_RANGE(_bytes) (1U<<(_bytes*8-1))
#define ARRAY_SIZE(_a) (sizeof((_a))/sizeof((_a)[0]))

#ifdef DEBUG
#   define BUILD_TYPE "DEBUG"
#else
#   define BUILD_TYPE "Release"
#endif

#define print_download(_dl, _st)  vLogCore(xDownload, __FILE__, __LINE__, "download %s: startOffset:%5i, endOffset:%5i, size:%6i, crc:0x%08lx\n", _st, (_dl)->startOffset, (_dl)->endOffset, (_dl)->endOffset-(_dl)->startOffset, (_dl)->crc)

#define STATS_PROTOCOL_VERSION 1
typedef enum
{
    LoggedIn = 0,
    IsFull   = 1,    
    IsLocked = 2,
    StatFlagsCount
} stat_flags_t;

#define isFull(stats) (((stats)->statFlags & (1<<IsFull)) != 0)
#define isLoggedIn(stats) (((stats)->statFlags & (1<<LoggedIn)) != 0)

typedef enum
{
    Clock     = 0,
    Asic      = 1,
    Asic1     = 2,
    Memory    = 3,
    Bluetooth = 4,
    Eeprom    = 5,    
    HwFlagsCount
} hw_flags_t;

typedef enum
{
    OldReadMoode = 0,
    UseLed       = 1,    
    ConfFlagsCount
} conf_t;

#define oldReadMode(config) (((config)->configFlags & (1<<OldReadMoode)) != 0)
#define useLed(config) (((config)->configFlags & (1<<UseLed)) != 0)

typedef struct
{
    uint32_t startOffset;
    uint32_t endOffset;
	uint32_t crc;
} download_t;


typedef struct
{
    uint8_t             protocolVersion      ;
    uint8_t             statFlags            ;
    uint8_t             hwFlags              ;
    uint32_t            serialNumber         ;
    uint32_t            memorySize           ;
    uint8_t             batteryPercentage    ;
    uint32_t            timestamp            ;
    uint32_t            memoryUsed           ;
    uint32_t            lifeSwipeCount       ;
    uint32_t            lifeKeysCount        ;
    uint16_t            swipeCount           ;
    uint16_t            keysCount            ;
    uint16_t            voltage              ;
    int16_t             dieTemperature       ;
    download_t          downloadParams       ;
} v001stat_t;

typedef struct
{
    uint32_t            configFlags;    
} v001conf_t;

typedef struct
{
    uint32_t offset;
} header_t;


extern v001stat_t stats;
extern v001conf_t config;
extern volatile uint32_t timestamp;

uint32_t timerUnits(void);

char *maptobin(uint32_t binary);
uint8_t count_bits(uint32_t v);
bool TimerExpired(uint32_t ms);
uint32_t GetTick(void);
CY_ISR(ClockTick);
uint32_t SuperFastHash (const void * data, int len) ;
void wdtReset(void);
void print_mac(uint32_t flags, CYBLE_GAP_BD_ADDR_T *device, uint16_t serviceUuid);
void print_auth_info(uint32_t flags, CYBLE_GAP_AUTH_INFO_T *info, char *s);
uint32_t crc32(const void *buf, int len, uint32_t crc);
void showStats(uint32_t flags, v001stat_t *stats);
void print_minute_mark(void);
char *cmd2st(int t);

#ifndef CYBLE_MSRVXXX_SERVICE_INDEX
    
/* Below are the indexes and handles of the defined Custom Services and their characteristics */
#define CYBLE_MSRVXXX_SERVICE_INDEX   (0x00u) /* Index of MSRvXXX service in the cyBle_customs array */
#define CYBLE_MSRVXXX_DEVICE_STATUS_CHAR_INDEX   (0x00u) /* Index of Device Status characteristic */
#define CYBLE_MSRVXXX_DEVICE_STATUS_CCCD_DESC_INDEX   (0x00u) /* Index of CCCD descriptor */
#define CYBLE_MSRVXXX_LOGIN_CHAR_INDEX   (0x01u) /* Index of Login characteristic */
#define CYBLE_MSRVXXX_DOWNLOAD_CHAR_INDEX   (0x02u) /* Index of Download characteristic */
#define CYBLE_MSRVXXX_DOWNLOAD_CCCD_DESC_INDEX   (0x00u) /* Index of CCCD descriptor */
#define CYBLE_MSRVXXX_ERASE_CHAR_INDEX   (0x03u) /* Index of Erase characteristic */
#define CYBLE_MSRVXXX_SETPASS_CHAR_INDEX   (0x04u) /* Index of SetPass characteristic */
#define CYBLE_MSRVXXX_SETKEY_CHAR_INDEX   (0x05u) /* Index of SetKey characteristic */
#define CYBLE_MSRVXXX_SETNAME_CHAR_INDEX   (0x06u) /* Index of SetName characteristic */
#define CYBLE_MSRVXXX_UPGRADE_CHAR_INDEX   (0x07u) /* Index of Upgrade characteristic */
#define CYBLE_MSRVXXX_SETTIME_CHAR_INDEX   (0x08u) /* Index of SetTime characteristic */
#define CYBLE_MSRVXXX_LOGFLAGS_CHAR_INDEX   (0x09u) /* Index of LogFlags characteristic */
#define CYBLE_MSRVXXX_REBOOT_CHAR_INDEX   (0x0Au) /* Index of Reboot characteristic */
#define CYBLE_MSRVXXX_MANUFACTURER_NAME_STRING_CHAR_INDEX   (0x0Bu) /* Index of Manufacturer Name String characteristic */
#define CYBLE_MSRVXXX_MODEL_NUMBER_STRING_CHAR_INDEX   (0x0Cu) /* Index of Model Number String characteristic */
#define CYBLE_MSRVXXX_SERIAL_NUMBER_STRING_CHAR_INDEX   (0x0Du) /* Index of Serial Number String characteristic */
#define CYBLE_MSRVXXX_HARDWARE_REVISION_STRING_CHAR_INDEX   (0x0Eu) /* Index of Hardware Revision String characteristic */
#define CYBLE_MSRVXXX_FIRMWARE_REVISION_STRING_CHAR_INDEX   (0x0Fu) /* Index of Firmware Revision String characteristic */


#define CYBLE_MSRVXXX_SERVICE_HANDLE   (0x000Cu) /* Handle of MSRvXXX service */
#define CYBLE_MSRVXXX_DEVICE_STATUS_DECL_HANDLE   (0x000Du) /* Handle of Device Status characteristic declaration */
#define CYBLE_MSRVXXX_DEVICE_STATUS_CHAR_HANDLE   (0x000Eu) /* Handle of Device Status characteristic */
#define CYBLE_MSRVXXX_DEVICE_STATUS_CCCD_DESC_HANDLE   (0x000Fu) /* Handle of CCCD descriptor */
#define CYBLE_MSRVXXX_LOGIN_DECL_HANDLE   (0x0010u) /* Handle of Login characteristic declaration */
#define CYBLE_MSRVXXX_LOGIN_CHAR_HANDLE   (0x0011u) /* Handle of Login characteristic */
#define CYBLE_MSRVXXX_DOWNLOAD_DECL_HANDLE   (0x0012u) /* Handle of Download characteristic declaration */
#define CYBLE_MSRVXXX_DOWNLOAD_CHAR_HANDLE   (0x0013u) /* Handle of Download characteristic */
#define CYBLE_MSRVXXX_DOWNLOAD_CCCD_DESC_HANDLE   (0x0014u) /* Handle of CCCD descriptor */
#define CYBLE_MSRVXXX_ERASE_DECL_HANDLE   (0x0015u) /* Handle of Erase characteristic declaration */
#define CYBLE_MSRVXXX_ERASE_CHAR_HANDLE   (0x0016u) /* Handle of Erase characteristic */
#define CYBLE_MSRVXXX_SETPASS_DECL_HANDLE   (0x0017u) /* Handle of SetPass characteristic declaration */
#define CYBLE_MSRVXXX_SETPASS_CHAR_HANDLE   (0x0018u) /* Handle of SetPass characteristic */
#define CYBLE_MSRVXXX_SETKEY_DECL_HANDLE   (0x0019u) /* Handle of SetKey characteristic declaration */
#define CYBLE_MSRVXXX_SETKEY_CHAR_HANDLE   (0x001Au) /* Handle of SetKey characteristic */
#define CYBLE_MSRVXXX_SETNAME_DECL_HANDLE   (0x001Bu) /* Handle of SetName characteristic declaration */
#define CYBLE_MSRVXXX_SETNAME_CHAR_HANDLE   (0x001Cu) /* Handle of SetName characteristic */
#define CYBLE_MSRVXXX_UPGRADE_DECL_HANDLE   (0x001Du) /* Handle of Upgrade characteristic declaration */
#define CYBLE_MSRVXXX_UPGRADE_CHAR_HANDLE   (0x001Eu) /* Handle of Upgrade characteristic */
#define CYBLE_MSRVXXX_SETTIME_DECL_HANDLE   (0x001Fu) /* Handle of SetTime characteristic declaration */
#define CYBLE_MSRVXXX_SETTIME_CHAR_HANDLE   (0x0020u) /* Handle of SetTime characteristic */
#define CYBLE_MSRVXXX_LOGFLAGS_DECL_HANDLE   (0x0021u) /* Handle of LogFlags characteristic declaration */
#define CYBLE_MSRVXXX_LOGFLAGS_CHAR_HANDLE   (0x0022u) /* Handle of LogFlags characteristic */
#define CYBLE_MSRVXXX_REBOOT_DECL_HANDLE   (0x0023u) /* Handle of Reboot characteristic declaration */
#define CYBLE_MSRVXXX_REBOOT_CHAR_HANDLE   (0x0024u) /* Handle of Reboot characteristic */
#define CYBLE_MSRVXXX_MANUFACTURER_NAME_STRING_DECL_HANDLE   (0x0025u) /* Handle of Manufacturer Name String characteristic declaration */
#define CYBLE_MSRVXXX_MANUFACTURER_NAME_STRING_CHAR_HANDLE   (0x0026u) /* Handle of Manufacturer Name String characteristic */
#define CYBLE_MSRVXXX_MODEL_NUMBER_STRING_DECL_HANDLE   (0x0027u) /* Handle of Model Number String characteristic declaration */
#define CYBLE_MSRVXXX_MODEL_NUMBER_STRING_CHAR_HANDLE   (0x0028u) /* Handle of Model Number String characteristic */
#define CYBLE_MSRVXXX_SERIAL_NUMBER_STRING_DECL_HANDLE   (0x0029u) /* Handle of Serial Number String characteristic declaration */
#define CYBLE_MSRVXXX_SERIAL_NUMBER_STRING_CHAR_HANDLE   (0x002Au) /* Handle of Serial Number String characteristic */
#define CYBLE_MSRVXXX_HARDWARE_REVISION_STRING_DECL_HANDLE   (0x002Bu) /* Handle of Hardware Revision String characteristic declaration */
#define CYBLE_MSRVXXX_HARDWARE_REVISION_STRING_CHAR_HANDLE   (0x002Cu) /* Handle of Hardware Revision String characteristic */
#define CYBLE_MSRVXXX_FIRMWARE_REVISION_STRING_DECL_HANDLE   (0x002Du) /* Handle of Firmware Revision String characteristic declaration */
#define CYBLE_MSRVXXX_FIRMWARE_REVISION_STRING_CHAR_HANDLE   (0x002Eu) /* Handle of Firmware Revision String characteristic */



#endif

#endif
