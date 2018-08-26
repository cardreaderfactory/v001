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


/*******************************************************************************
* File Name: app_Ble.h
*
* Description:
*  Contains the function prototypes and constants available to the example
*  project.
*
********************************************************************************/


#if !defined(_BLE_H)
    
    #define _BLE_H
    
    #include "stdint.h"
    #include "stdbool.h"    
    
    typedef struct 
    {
        volatile bool     uploading;
        uint32_t currentOffset;
        uint32_t retry_ts;
        uint32_t retry_count;
        uint32_t bytes;  
        uint32_t start_ts;
        uint32_t last_ts;
        uint32_t delay;
        uint32_t ok_ts;
        uint32_t ok_count;
    } upload_t;

     
    /***************************************
    *       Function Prototypes
    ***************************************/
    void Ble_Stack_Handler( uint32 code, void *param);
    //void * Ble_Stack_Handler( uint32_t code, void *param);
    void HandleBleProcessing(void);
    void attrHandleInit(void);
    void enableNotifications(void);
    void sendStatsNotification(void);
    void upload(bool u);
    void reboot(void);
    
    extern upload_t up;
    
#endif

/* [] END OF FILE */
