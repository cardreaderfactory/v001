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
* File Name: client.c
*
* Version: 1.0
*
* Description:
*  Common BLE application code for client devices.
*
*/

#include <project.h>
#include "v001.h"
#include "debug_central.h"

#define CONNECT 0x01
uint8 flag = 0;
uint8 advDevices = 0u;
uint8 deviceN = 0u;
CYBLE_UUID16 serviceUuid = 0x0000u;



void StartScan(CYBLE_UUID16 uuid)
{
    serviceUuid = uuid;
    apiResult = CyBle_GapcStartScan(CYBLE_SCANNING_FAST);
	if(apiResult != CYBLE_ERROR_OK)
    {
        vLog(xBle,"StartScan API Error: %xd \n", apiResult);
    }
    else
    {
        vLog(xBle,"Start Scan \n");
    }
}

void ScanEventHandler(CYBLE_GAPC_ADV_REPORT_T* eventParam)
{
#if 0    
    uint8 newDevice = 0u, device = 0u;
    uint8 i;
    uint8 adStructPtr = 0u;
    uint8 adTypePtr = 0u;
    uint8 nextPtr = 0u;

    vLog(xBle,"SCAN_PROGRESS_RESULT: peerAddrType - %d ", eventParam->peerAddrType);
    xprintf("peerBdAddr: ");
    for(newDevice = 1u, i = 0u; i < advDevices; i++)
    {
        if((memcmp(peerAddr[i].bdAddr, eventParam->peerBdAddr, CYBLE_GAP_BD_ADDR_SIZE) == 0)) /* same address */
        {
            device = i;
            xprintf("%x: ", device);
            newDevice = 0u;
            break;
        }
    }
    if(newDevice != 0u)
    {
        if(advDevices < CYBLE_MAX_ADV_DEVICES)
        {
            memcpy(peerAddr[advDevices].bdAddr, eventParam->peerBdAddr, CYBLE_GAP_BD_ADDR_SIZE);
            peerAddr[advDevices].type = eventParam->peerAddrType;
            device = advDevices;
            advDevices++;
            xprintf("%02x: ", device);

        }
    }
    for(i = CYBLE_GAP_BD_ADDR_SIZE; i > 0u; i--)
    {
        xprintf("%02x", eventParam->peerBdAddr[i-1]);
    }
    xprintf(", rssi - %d dBm, data - ", eventParam->rssi);

    /* Print and parse advertisement data and connect to device which has HRM */
    adStructPtr = 0u;
    for(i = 0; i < eventParam->dataLen; i++)
    {
        xprintf("%02x", eventParam->data[i]);

        if(i == adStructPtr)
        {
            adTypePtr = i + 1;
            adStructPtr += eventParam->data[i] + 1;
            nextPtr = 1;
        }
        else if(i == (adTypePtr + nextPtr))
        {
            switch(eventParam->data[adTypePtr])
            {
                case CYBLE_GAP_ADV_FLAGS:
                    break;

                case CYBLE_GAP_ADV_INCOMPL_16UUID:
                case CYBLE_GAP_ADV_COMPL_16UUID:
                    if(serviceUuid == CyBle_Get16ByPtr(&(eventParam->data[i])))
                    {
                        newDevice = 2; /* temporary use newDevice as a flag */
                    }
                    else
                    {
                        nextPtr += 2;
                    }
                    break;

                default:
                    break;
            }
        }
    }

    if(2 == newDevice)
    {
        deviceN = device;
        xprintf("          This device contains service UUID: %x\n",serviceUuid);
    }
    else
    {
        xprintf("\n");
    }
#endif    
}

void ClientDebugOut(uint32 event, void* eventParam)
{
    uint16 length, i;
    CYBLE_GATTC_GRP_ATTR_DATA_LIST_T *locAttrData;
    uint8 type, *locHndlUuidList;

    switch(event)
    {
        case CYBLE_EVT_GAPC_SCAN_PROGRESS_RESULT:
            ScanEventHandler((CYBLE_GAPC_ADV_REPORT_T *)eventParam);
            break;

        case CYBLE_EVT_GAPC_SCAN_START_STOP:
            vLog(xBle,"EVT_GAPC_SCAN_START_STOP, state: %x\n", CyBle_GetState());
            break;

        case CYBLE_EVT_GAP_DEVICE_CONNECTED:
            //CyBle_GapAddDeviceToWhiteList(&peerAddr[deviceN]);
            break;

        /**********************************************************
        *                       GATT Events
        ***********************************************************/

        case CYBLE_EVT_GATTC_ERROR_RSP:
            vLog(xBle,"EVT_GATTC_ERROR_RSP: opcode: ");
            switch(((CYBLE_GATTC_ERR_RSP_PARAM_T *)eventParam)->opCode)
            {
                case CYBLE_GATT_FIND_INFO_REQ:
                    vLog(xBle,"FIND_INFO_REQ");
                    break;

                case CYBLE_GATT_READ_BY_TYPE_REQ:
                    vLog(xBle,"READ_BY_TYPE_REQ");
                    break;

                case CYBLE_GATT_READ_BY_GROUP_REQ:
                    vLog(xBle,"READ_BY_GROUP_REQ");
                    break;

                default:
                    vLog(xBle,"%x", ((CYBLE_GATTC_ERR_RSP_PARAM_T *)eventParam)->opCode);
                    break;
            }
            vLog(xBle,",  handle: %x,  ", ((CYBLE_GATTC_ERR_RSP_PARAM_T *)eventParam)->attrHandle);
            vLog(xBle,"errorcode: ");
            switch(((CYBLE_GATTC_ERR_RSP_PARAM_T *)eventParam)->errorCode)
            {
                case CYBLE_GATT_ERR_ATTRIBUTE_NOT_FOUND:
                    vLog(xBle,"ATTRIBUTE_NOT_FOUND");
                    break;

                case CYBLE_GATT_ERR_READ_NOT_PERMITTED:
                    vLog(xBle,"READ_NOT_PERMITTED");
                    break;

                default:
                    vLog(xBle,"%x", ((CYBLE_GATTC_ERR_RSP_PARAM_T *)eventParam)->errorCode);
                    break;
            }
            vLog(xBle,"\n");
            break;

        case CYBLE_EVT_GATTC_READ_RSP:
            vLog(xBle,"EVT_GATTC_READ_RSP: ");
            length = ((CYBLE_GATTC_READ_RSP_PARAM_T *)eventParam)->value.len;
            for(i = 0; i < length; i++)
            {
                vLog(xBle,"%2.2x ", ((CYBLE_GATTC_READ_RSP_PARAM_T *)eventParam)->value.val[i]);
            }
            vLog(xBle,"\n");
            break;

        case CYBLE_EVT_GATTC_WRITE_RSP:
            vLog(xBle,"EVT_GATTC_WRITE_RSP: ");
            vLog(xBle,"bdHandle: %x \n", ((CYBLE_CONN_HANDLE_T *)eventParam)->bdHandle);
            break;

        case CYBLE_EVT_GATTC_XCHNG_MTU_RSP:
            vLog(xBle,"EVT_GATTC_XCHNG_MTU_RSP \n");
            break;

        case CYBLE_EVT_GATTC_READ_BY_GROUP_TYPE_RSP: /* Response to CYBLE_DiscoverAllPrimServices() */
            vLog(xBle,"EVT_GATTC_READ_BY_GROUP_TYPE_RSP: ");
            locAttrData = &(*(CYBLE_GATTC_READ_BY_GRP_RSP_PARAM_T *)eventParam).attrData;
            for(i = 0u; i < locAttrData -> attrLen; i ++)
            {
                vLog(xBle,"%2.2x ",*(uint8 *)(locAttrData->attrValue + i));
            }
            vLog(xBle,"\n");
            break;

        case CYBLE_EVT_GATTC_READ_BY_TYPE_RSP:      /* Response to CYBLE_DiscoverAllCharacteristicsOfService() */
            vLog(xBle,"EVT_GATTC_READ_BY_TYPE_RSP: ");
            locAttrData = &(*(CYBLE_GATTC_READ_BY_TYPE_RSP_PARAM_T *)eventParam).attrData;
            for(i = 0u; i < locAttrData -> attrLen; i ++)
            {
                vLog(xBle,"%2.2x ",*(uint8 *)(locAttrData->attrValue + i));
            }
            vLog(xBle,"\n");
            break;

        case CYBLE_EVT_GATTC_FIND_INFO_RSP:
            vLog(xBle,"EVT_GATTC_FIND_INFO_RSP: ");
            type = (*(CYBLE_GATTC_FIND_INFO_RSP_PARAM_T *)eventParam).uuidFormat;
            locHndlUuidList = (*(CYBLE_GATTC_FIND_INFO_RSP_PARAM_T *)eventParam).handleValueList.list;
            if(type == CYBLE_GATT_16_BIT_UUID_FORMAT)
            {
                length = CYBLE_ATTR_HANDLE_LEN + CYBLE_GATT_16_BIT_UUID_SIZE;
            }
            else
            {
                length = CYBLE_ATTR_HANDLE_LEN + CYBLE_GATT_128_BIT_UUID_SIZE;
            }

            for(i = 0u; i < (*(CYBLE_GATTC_FIND_INFO_RSP_PARAM_T *)eventParam).handleValueList.byteCount; i += length)
            {
                if(type == CYBLE_GATT_16_BIT_UUID_FORMAT)
                {
                    vLog(xBle,"%2.2x ",CyBle_Get16ByPtr(locHndlUuidList + i + CYBLE_ATTR_HANDLE_LEN));
                }
                else
                {
                    vLog(xBle,"UUID128");
                }
            }
            vLog(xBle,"\n");
            break;

        case CYBLE_EVT_GATTC_FIND_BY_TYPE_VALUE_RSP:
            vLog(xBle,"EVT_GATTC_FIND_BY_TYPE_VALUE_RSP \n");
            break;

        case CYBLE_EVT_GATTC_HANDLE_VALUE_NTF:
            vLog(xBle,"EVT_GATTC_HANDLE_VALUE_NTF \n");
            break;

        /**********************************************************
        *                       Discovery Events
        ***********************************************************/
        case CYBLE_EVT_GATTC_SRVC_DUPLICATION:
            vLog(xBle,"EVT_GATTC_SRVC_DUPLICATION, UUID: %x \n", *(uint16 *)eventParam);
            break;

        case CYBLE_EVT_GATTC_CHAR_DUPLICATION:
            vLog(xBle,"EVT_GATTC_CHAR_DUPLICATION, UUID: %x \n", *(uint16 *)eventParam);
            break;

        case CYBLE_EVT_GATTC_DESCR_DUPLICATION:
            vLog(xBle,"EVT_GATTC_DESCR_DUPLICATION, UUID: %x \n", *(uint16 *)eventParam);
            break;

        case CYBLE_EVT_GATTC_SRVC_DISCOVERY_FAILED:
            vLog(xBle,"EVT_GATTC_DISCOVERY_FAILED \n");
            break;

        case CYBLE_EVT_GATTC_SRVC_DISCOVERY_COMPLETE:
            vLog(xBle,"EVT_GATTC_SRVC_DISCOVERY_COMPLETE \n");
            break;

        case CYBLE_EVT_GATTC_INCL_DISCOVERY_COMPLETE:
            vLog(xBle,"EVT_GATTC_INCL_DISCOVERY_COMPLETE \n");
            break;

        case CYBLE_EVT_GATTC_CHAR_DISCOVERY_COMPLETE:
            vLog(xBle,"EVT_GATTC_CHAR_DISCOVERY_COMPLETE \n");
            break;

        case CYBLE_EVT_GATTC_DISCOVERY_COMPLETE:
            vLog(xBle,"EVT_GATTC_DISCOVERY_COMPLETE \n");
            break;


        default:
        //#if (0)
            vLog(xBle,"unknown event: %lx \n", event);
        //#endif
            break;
    }
}


void BleDebugOut(uint32 event, void* eventParam)
{
    switch(event)
    {
        case CYBLE_EVT_STACK_ON:
            vLog(xBle,"EVT_STACK_ON \n");
            break;

        case CYBLE_EVT_STACK_BUSY_STATUS:
            vLog(xBle,"EVT_STACK_BUSY_STATUS \n");
            break;

        case CYBLE_EVT_TIMEOUT: /* 0x01 -> GAP limited discoverable mode timeout. */
                                /* 0x02 -> GAP pairing process timeout. */
                                /* 0x03 -> GATT response timeout. */
            vLog(xBle,"EVT_TIMEOUT: %d \n", *(uint8 *)eventParam);
            break;

        case CYBLE_EVT_HARDWARE_ERROR:    /* This event indicates that some internal HW error has occurred. */
            vLog(xBle,"EVT_HARDWARE_ERROR \n");
            break;

        case CYBLE_EVT_HCI_STATUS:
            vLog(xBle,"EVT_HCI_STATUS \n");
            break;

            
        /**********************************************************
        *                       GAP Events
        ***********************************************************/

        case CYBLE_EVT_GAP_AUTH_REQ:
            vLog(xBle,"EVT_GAP_AUTH_REQ \n");
            break;

        case CYBLE_EVT_GAP_PASSKEY_ENTRY_REQUEST:
            vLog(xBle,"EVT_GAP_PASSKEY_ENTRY_REQUEST \n");
            break;

        case CYBLE_EVT_GAP_PASSKEY_DISPLAY_REQUEST:
            vLog(xBle,"EVT_GAP_PASSKEY_DISPLAY_REQUEST: %d \n", (int) (*(CYBLE_GAP_PASSKEY_DISP_INFO_T *)eventParam).passkey);
            break;

        case CYBLE_EVT_GAP_AUTH_FAILED:
            vLog(xBle,"EVT_GAP_AUTH_FAILED, reason: ");
            switch(*(CYBLE_GAP_AUTH_FAILED_REASON_T *)eventParam)
            {
                case CYBLE_GAP_AUTH_ERROR_CONFIRM_VALUE_NOT_MATCH:
                    vLog(xBle,"CONFIRM_VALUE_NOT_MATCH\n");
                    break;
                    
                case CYBLE_GAP_AUTH_ERROR_INSUFFICIENT_ENCRYPTION_KEY_SIZE:
                    vLog(xBle,"INSUFFICIENT_ENCRYPTION_KEY_SIZE\n");
                    break;
                
                case CYBLE_GAP_AUTH_ERROR_UNSPECIFIED_REASON:
                    vLog(xBle,"UNSPECIFIED_REASON\n");
                    break;
                    
                case CYBLE_GAP_AUTH_ERROR_AUTHENTICATION_TIMEOUT:
                    vLog(xBle,"AUTHENTICATION_TIMEOUT\n");
                    break;
                    
                default:
                    vLog(xBle,"0x%x  \n", *(CYBLE_GAP_AUTH_FAILED_REASON_T *)eventParam);
                    break;
            }
            break;

        case CYBLE_EVT_GAP_DEVICE_CONNECTED:
            vLog(xBle,"EVT_GAP_DEVICE_CONNECTED: %x \n", cyBle_connHandle.bdHandle);
            break;

        case CYBLE_EVT_GAPC_CONNECTION_UPDATE_COMPLETE:
            vLog(xBle,"EVT_GAPC_CONNECTION_UPDATE_COMPLETE \n");
            break;

        case CYBLE_EVT_GAP_DEVICE_DISCONNECTED:
            vLog(xBle,"EVT_GAP_DEVICE_DISCONNECTED, reason: %x \n", *(uint8*)eventParam);
            break;

        case CYBLE_EVT_GAP_AUTH_COMPLETE:
            vLog(xBle,"EVT_GAP_AUTH_COMPLETE: security:%x, bonding:%x, ekeySize:%x, authErr %x \n",
                        ((CYBLE_GAP_AUTH_INFO_T *)eventParam)->security,
                        ((CYBLE_GAP_AUTH_INFO_T *)eventParam)->bonding, 
                        ((CYBLE_GAP_AUTH_INFO_T *)eventParam)->ekeySize, 
                        ((CYBLE_GAP_AUTH_INFO_T *)eventParam)->authErr);
            break;

        case CYBLE_EVT_GAP_ENCRYPT_CHANGE:
            vLog(xBle,"EVT_GAP_ENCRYPT_CHANGE: %d \n", *(uint8 *)eventParam);
            break;


        /**********************************************************
        *                       GATT Events
        ***********************************************************/
        case CYBLE_EVT_GATTC_ERROR_RSP:
            vLog(xBle,"EVT_GATTC_ERROR_RSP: opcode: ");
            switch(((CYBLE_GATTC_ERR_RSP_PARAM_T *)eventParam)->opCode)
            {
                case CYBLE_GATT_FIND_INFO_REQ:
                    vLog(xBle,"FIND_INFO_REQ");
                    break;

                case CYBLE_GATT_READ_BY_TYPE_REQ:
                    vLog(xBle,"READ_BY_TYPE_REQ");
                    break;

                case CYBLE_GATT_READ_BY_GROUP_REQ:
                    vLog(xBle,"READ_BY_GROUP_REQ");
                    break;

                default:
                    vLog(xBle,"%x", ((CYBLE_GATTC_ERR_RSP_PARAM_T *)eventParam)->opCode);
                    break;
            }
            vLog(xBle,",  handle: %x,  ", ((CYBLE_GATTC_ERR_RSP_PARAM_T *)eventParam)->attrHandle);
            vLog(xBle,"errorcode: ");
            switch(((CYBLE_GATTC_ERR_RSP_PARAM_T *)eventParam)->errorCode)
            {
                case CYBLE_GATT_ERR_ATTRIBUTE_NOT_FOUND:
                    vLog(xBle,"ATTRIBUTE_NOT_FOUND");
                    break;

                default:
                    vLog(xBle,"%x", ((CYBLE_GATTC_ERR_RSP_PARAM_T *)eventParam)->errorCode);
                    break;
            }
            vLog(xBle,"\n");
            break;

        case CYBLE_EVT_GATT_CONNECT_IND:
            vLog(xBle,"EVT_GATT_CONNECT_IND: attId %x, bdHandle %x \n", 
                ((CYBLE_CONN_HANDLE_T *)eventParam)->attId, ((CYBLE_CONN_HANDLE_T *)eventParam)->bdHandle);
            break;

        case CYBLE_EVT_GATT_DISCONNECT_IND:
            vLog(xBle,"EVT_GATT_DISCONNECT_IND \n");
            break;


        /**********************************************************
        *                       L2CAP Events
        ***********************************************************/

        case CYBLE_EVT_L2CAP_CONN_PARAM_UPDATE_REQ:
            vLog(xBle,"EVT_L2CAP_CONN_PARAM_UPDATE_REQ \n");
            break;

            
        /**********************************************************
        *                       Debug Events
        ***********************************************************/

        case CYBLE_DEBUG_EVT_BLESS_INT:
            break;


        default:
            ClientDebugOut(event, eventParam);
            break;
    }
}


/* [] END OF FILE */
