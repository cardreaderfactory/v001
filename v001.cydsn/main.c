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
#include "bluetooth.h"
#include "memCyFlash.h"
#include "memory.h"
#include "profiler.h"
#include "adc.h"


uint8_t mode = 0;

uint32_t readBuild(void)
{
    uint32_t buf[2];
    CyGetUniqueId(buf);
    
    return SuperFastHash(buf,8);
}


void initStats(void)
{
    memset(&stats, 0, sizeof(stats));    
    stats.protocolVersion = STATS_PROTOCOL_VERSION;
    stats.serialNumber = readBuild();
}


void menu(void)
{
    char command = UART_UartGetChar();
    if (command >= '0' && command <= '9')
    {
#if MODULES & MOD_FLASH_TEST_WRITE
        if (mode == 1)
        {
             char buf[256];
//                if (buf[0] != 1)
//                {
//                    for(int i = 0; i < sizeof(buf); i++)
//                        buf[i] = i+1;
//                }
            
            if (command == '0')
            {
                    vLog(xDebug, "flushing\n");
                    flush();                    
            }
            else
            {
                uint8_t test_data[] = {13,52,31,32,33,127,128,129,255};
                uint8_t bytes = 0;
                command = command - '0' - 1;
                if (command >= 0 && command < ARRAY_SIZE(test_data))
                    bytes = test_data[(uint8_t)command];
//                    vLog(xDebug, "writing %i bytes\n", bytes);
                
                char *a = buf;
                if (bytes > 8)
                {
                    memset(buf, ' ', sizeof(buf));
                    xsnprintf(a, sizeof(buf)-1, "{%i,%03i}[", command, bytes); // printing 8 bytes
                    a+=8; // increasing with 8 bytes
                    int i = 0;
                    while (a < buf + bytes-3)
                    {
                        xsprintf(a, "%3i", i);
                        a+=3;
                        *a = ' ';
                        i++;
                    }
                    buf[bytes-1] = ']'; // printing one more byte. total 9 bytes.
                }
                else
                {
                    memset(buf, command+'0', bytes);
                    if (bytes > 1)
                    {
                        buf[0] = '[';
                        buf[bytes-1] = ']';
                    }                            
                }
                
                write(buf, bytes);                    
            }
        }
#endif
#if MODULES & MOD_FLASH_VIEW
        else if (mode == 2)
        {
            #define VIEW_ROW_COUNT 8                
            uint32_t page;
            uint32_t count;                               
            
            if (command == '9')
            {
                page = 0;                    
                count = 1+(uint32_t)(flashWp()-flashSp())/CY_FLASH_SIZEOF_ROW;
            }
            else
            {
                page = (command-'0') * VIEW_ROW_COUNT;                    
                count = VIEW_ROW_COUNT;
            }
            
            page += (uint32_t)flashSp() / CY_FLASH_SIZEOF_ROW;
            
            
            xprintf("pages %lu to %lu (memory %lu to %lu aka 0x%lx to 0x%lx)\n", 
                page, 
                page + count, 
                rowAddr(page), 
                rowAddr(page+1), 
                rowAddr(page), 
                rowAddr(page+1));
            for(uint32_t i = 0; i < count && page < CY_FLASH_NUMBER_ROWS; i++, page++)
            {                    
                flash_printPage(page);
            }
            
        }
#endif            
    }
//    profiler_breakPoint("user interface");
    
    switch (command)
    {            
        case 'o':
            profiler_showReport();
            //profiler_start();
            break;
        case 'r':
            reboot();
            break;
        
        case 'x':
            vLog(xBle, "disconnecting\n");
            CyBle_GapDisconnect(cyBle_connHandle.bdHandle);
            profiler_breakPoint("CyBle_GapDisconnect");
            break;
        case 'a':
            adc_info();
            break;
        case 'l':                
            vLogFlags ^= (1<<xDownload);
            vLog(xInfo, "download logs: %i\n", (vLogFlags & (1<<xDownload)) != 0);
            break;
        case 'b':                
            vLogFlags ^= (1<<xBle);
            vLog(xInfo, "bluetooth logs: %i\n", (vLogFlags & (1<<xBle)) != 0);
            break;
        case 's':                
            sendStatsNotification();                
//                vLog(xInfo, "Notification sent\n");
//                {
//                    v001stat_t *s = &stats;
//                    //vLog(xBle, "name: %s\n", s->btName);
//                    vLog(xBle, "serial: %lu %06z\n", s->serialNumber, s->serialNumber);                
//                }
            profiler_breakPoint("sendStatsNotification");
            break;
#if MODULES & MOD_CY_FLASH_FILL                    
        case 'f':
            vLog(xInfo, "fill flash ...\n");
            flashFill();
            profiler_breakPoint("flashFill");
            break;
#endif                
        case 'e':
            vLog(xInfo, "erasing flash ...\n");
            flashErase();
            profiler_breakPoint("flashErase");
            break;
#if MODULES & MOD_FLASH_TEST_WRITE
        case 'w':
            mode = 1;
            vLog(xInfo, "mode: write. press 1..9 to write data\n");
            break;
#endif
#if MODULES & MOD_FLASH_VIEW
        case 'v':
            mode = 2;
            vLog(xInfo, "mode: view. press 1..9 to view pages\n");
            break;
#endif                
        case 'c':
            upload(false);
            break;
            
#if MODULES & MOD_PEAK_DETECT_UNIT_TEST
        case 'p':
            peak_test();
            break;
#endif                
            
//            case '-':
//                vLog(xInfo, "resending one package\n");
//                up.pkt_id--;
//                break;
//            case '=':
//                vLog(xInfo, "skipping one package\n");
//                up.pkt_id++;
//                break;
            
            
            
    }

}

void reboot(void)
{
    profiler_showReport();                
    vLog(xInfo, "rebooting....\n");
    UART_UartPutChar('\n');
    UART_UartPutChar('\n');
    while (UART_SpiUartGetTxBufferSize() != 0 && UART_GET_TX_FIFO_SR_VALID != 0);
    CySoftwareReset();
}
        
int main()
{
    
    #ifdef LOW_POWER_MODE    
//        CYBLE_LP_MODE_T         lpMode;
//        CYBLE_BLESS_STATE_T     blessState;
    #endif
    
    CYBLE_API_RESULT_T      bleApiResult;   
    
    uint8 resetCause = 0;
    
    /*===========================================================================================
     * this code piece detects the reset cause, if the last reset is caused by watchdog, a red LED
     * indicator is turn on
     *==========================================================================================*/
    /* Get reset cause after system is powered */
    resetCause = CySysGetResetReason(CY_SYS_RESET_WDT | CY_SYS_RESET_SW | CY_SYS_RESET_PROTFAULT);
    
    
    Notification_LED_Write(LED_OFF);
    Error_LED_Write(LED_OFF);          
    initStats();
        
    /* initialize watchdog */
    ClockTick_ISR_SetPriority(0);
    ClockTick_ISR_StartEx(ClockTick);          
    
#ifdef DEBUG
    CySysWdtSetMode(0, CY_SYS_WDT_MODE_INT); // disable the watchdog reset
#else
    CySysWdtSetMode(0, CY_SYS_WDT_MODE_INT_RESET);    
#endif    
    
    /* enable global interrupt */
    CyGlobalIntEnable;   

//    CyGetUniqueId(id);

    setTime(NULL);
    
    UART_Start();   
    xdev_out(myputchar);
    print_boot_complete();
    profiler_start();
    CyFlash_Start();
    adc_start();
    //xprintf("CyGetUniqueID = 0x%08lx 0x%08lx\n", id[0], id[1]);
       
    
    if(resetCause == CY_SYS_RESET_WDT)
    {   
        vLog(xBUG, " ---- Last reset cause was the watchdog !!! ----\n");
        //writeResetReason();        
    }
    
    vLog(xDebug, "tu2ms(32768) = %i\n", tu2ms(32768));
    vLog(xDebug, "tu2ms(5*32768) = %i\n", tu2ms(5*32768));    
    
    //xprintf("Stats: %i\n", sizeof(stats));
    showStats(xInfo, &stats);
    
    bleApiResult = CyBle_Start(Ble_Stack_Handler);     
    assert(CYBLE_ERROR_OK == bleApiResult);

//    aesTest();
//    CYBLE_STACK_LIB_VERSION_T   stackVersion;
//    bleApiResult = CyBle_GetStackLibraryVersion(&stackVersion);
//    assert(CYBLE_ERROR_OK == bleApiResult);   
//    xprintf("Stack Version: %d.%d.%d.%d\n", stackVersion.majorVersion, 
//        stackVersion.minorVersion, stackVersion.patch, stackVersion.buildNumber);
                     
    for(;;)
    {
        profiler_breakPoint("Not profiled");
        CyBle_ProcessEvents();
        profiler_breakPoint("CyBle_ProcessEvents");        
        HandleBleProcessing();
        profiler_breakPoint("HandleBleProcessing");        
        menu();
        adc_main();
        print_minute_mark();

//        profiler_breakPoint("user interface");
        
        wdtReset();
//        profiler_breakPoint("wdtReset");
    }
    
}

/* [] END OF FILE */
