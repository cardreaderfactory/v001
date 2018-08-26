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

#include <project.h>
#include "device.h"
#include "bluetooth.h"
#include "profiler.h"
#include "menu.h"

int main()
{
    CYBLE_API_RESULT_T      bleApiResult;
    
    uint8 resetCause = 0;
    
    /*===========================================================================================
     * this code piece detects the reset cause, if the last reset is caused by watchdog, a red LED
     * indicator is turn on
     *==========================================================================================*/
    /* Get reset cause after system is powered */
    resetCause = CySysGetResetReason(CY_SYS_RESET_WDT | CY_SYS_RESET_SW | CY_SYS_RESET_PROTFAULT);
    
    
    Notification_LED_Write(LED_OFF);    
    //initStats();
        
//    CyIntSetSysVector(SYSTICK_INTERRUPT_VECTOR_NUMBER, SysTick_ISR);  /* Point the Systick vector to the ISR */ 
//    SysTick_Config(CYDEV_BCLK__HFCLK__HZ / INTERRUPT_FREQ);      /* Set the number of ticks between interrupts */     
    
        
    /* initialize watchdog */
    ClockTick_ISR_StartEx(ClockTick);          
    ClockTick_ISR_SetPriority(0);
    
    /* enable global interrupt */
    CyGlobalIntEnable;   

    CySysWdtResetCounters(CY_SYS_WDT_COUNTER0_RESET);
    while(CySysWdtGetCount(0) < 32768/1024*20); // wait 20 ms - time we want the led to be turned off...
    CySysWdtResetCounters(CY_SYS_WDT_COUNTER1_RESET);

    
    UART_Start();   
    xdev_out(myputchar);

    print_boot_complete();
//    xprintf("Welcome to " DEVICE_NAME " " BUILD_TYPE "\n");

    if(resetCause == CY_SYS_RESET_WDT)
    {   
        vLog(xBUG, " ---- Last reset cause was the watchdog !!! ----\n");
        //writeResetReason();        
    }
    profiler_start();
   
    bleApiResult = CyBle_Start(Ble_Stack_Handler);     
    assert(CYBLE_ERROR_OK == bleApiResult);

        
//    CYBLE_STACK_LIB_VERSION_T   stackVersion;
//    bleApiResult = CyBle_GetStackLibraryVersion(&stackVersion);
//    assert(CYBLE_ERROR_OK == bleApiResult);   
//    xprintf("Stack Version: %d.%d.%d.%d\n", stackVersion.majorVersion, 
//        stackVersion.minorVersion, stackVersion.patch, stackVersion.buildNumber);
                 
#ifdef DEBUG
    check_duplicate_menu_keys(); // menu key check
#endif        
    for(;;)
    {
        main_loop();
    }    
}


void main_loop(void)
{    
    CyBle_ProcessEvents();
    HandleBleProcessing();
    print_minute_mark();
    menu();
    wdtReset();        
}

