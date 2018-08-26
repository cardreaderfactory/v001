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
   
    #include "device.h"    
    #include "uart.h"
    #include "stdbool.h"

    extern int pending_stats_read;
    extern uint32_t log_sync;                
    /***************************************
    *       Function Prototypes
    ***************************************/
    void Ble_Stack_Handler( uint32_t code, void *param);
    void HandleBleProcessing(void);
    void attrHandleInit(void);
    void enableNotifications(void);
    void cancelDownload(char *st);
    bool sendCommand(CYBLE_GATT_DB_ATTR_HANDLE_T command, void *args, uint8 len);
    uint32_t download(uint32_t start, uint32_t len);
    uint16_t maxBlockSize(void);    

    bool readStats(void);
    bool readDownloadParams(void);
    bool readParams(CYBLE_GATTC_READ_REQ_T handle);
    bool yield(uint32_t delay, volatile int *semaphore);
    bool isDownloading(void);
    bool writeLogFlags(uint32_t flags);
    
    
#endif

/* [] END OF FILE */
