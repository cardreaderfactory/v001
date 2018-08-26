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


#define LOG_CASE(__A) case __A: vLog(xBle, __stringnify(__A) "\n");
#define BAN_SECONDS (10*60)
//#define DEBUG_SEND_DELAY

/*
 *             speed (kb/s)               |         ram        
 *   mtu    1buffer    2buffers 3buffers  |  1buffer  2buffers  3buffers
 *   263         16        25.6     30.5  |    13984     14256     14536
 *   512       26.8        33.6     32.7  |    14720     15248     15776
 *
 */


volatile static bool peerDeviceFound         = false;
//volatile bool indicationConfirmed = true;
volatile bool bleConnected = false;
volatile bool userStoppedScan = false;
bool fuckedup = false;
bool erase_pending = false;
uint32_t clearBanTs = 0;
int authFailed = 0;
download_t *dl = &stats.downloadParams;

uint16_t uploadCccd = 0;
uint16_t statsCccd = 0;

uint32_t writeRspTs  = 0;   
uint8_t  writeRspCnt = 0;

upload_t up;

//uint8 securityKey[] = "1234567890123456";

/* MTU size to be used by Client and Server after MTU exchange */
uint16 mtuSize      = CYBLE_GATT_MTU;

CYBLE_GAP_BD_ADDR_T             peerAddr;           /* BD address of the peer device */
CYBLE_CONN_HANDLE_T             connHandle;

CYBLE_GAP_AUTH_INFO_T peerAuthInfo;

/* UUID of the custom BLE UART service */
const uint32 bleMSRvServiceUuid = 0x0a55; 

/* structure to be passed for discovering service by UUID */
//const CYBLE_GATT_VALUE_T    bleMSRvServiceUuidInfo = {
//                                                        (uint8 *) &bleMSRvServiceUuid,
//                                                        CYBLE_GATT_16_BIT_UUID_SIZE,
//                                                        CYBLE_GATT_16_BIT_UUID_SIZE
//                                                      };
//
void calc_crc_dl(download_t *d);

uint32_t maxBlockSize(void)
{
    return (mtuSize - 3 - sizeof(header_t)) / 4 * 4;
}

#ifdef DEBUG_SEND_DELAY
//uint32_t uts = 0;
uint32_t udelay = 0;
uint16_t ucnt = 0;
uint16_t pcnt = 0;
#endif    

void upload(bool u)
{
    if (u == up.uploading)
        return;
    if (u)
    {
        memset(&up, 0, sizeof(up));
        up.currentOffset = dl->startOffset;
//        indicationConfirmed = true;
        
        up.last_ts = GetTick();
        up.start_ts = GetTick();
//        up.delay = 0;
        up.delay = 15; // delaying 15ms here increases the transfer speed
    }
    else //if (up.currentOffset == dl->endOffset)
    {
#ifdef DEBUG_SEND_DELAY
        if (vLogFlags & (1<<xDownload))
            vLog(xDebug, "delayed by stack: %lums (%i times from %i packets)\n", udelay, ucnt, pcnt);
        else
            myputchar('\n');
        udelay = 0;        
#endif        
        uint32_t ts = GetTick();
        //calc_crc_dl(dl);
        vLog(xInfo, "Upload complete (%lu bytes, %i.%ikb/sec, %lums) crc: 0x%08x\n", 
            //up.currentOffset - dl->startOffset, 
            up.bytes, 
            up.bytes / (ts - up.start_ts), 
            (10 * up.bytes / (ts - up.start_ts))%10, 
            (ts - up.start_ts) * 1000 / 1024,                        
            dl->crc);              
    }
    
    up.uploading = u;
//    uts = GetTick();
}


void resetAllFlags()
{
    /* RESET all flags */
    bleConnected = false;
    upload(false);
    peerDeviceFound         = false;
//    indicationConfirmed     = true;
    statsCccd = 0;
    uploadCccd = 0;
}

void print_peer(CYBLE_GAPC_ADV_REPORT_T *advReport, uint16_t serviceUuid)
{
    xprintf("peer mac: %02x %02x %02x %02x %02x %02x, rssi: %d, service: %04x\n",
                advReport->peerBdAddr[5],
                advReport->peerBdAddr[4],
                advReport->peerBdAddr[3],
                advReport->peerBdAddr[2],
                advReport->peerBdAddr[1],
                advReport->peerBdAddr[0],
                (int32_t)advReport->rssi,
                serviceUuid);

    // advReport->rssi -> signal
    // advReport->eventType -> directed advertising, scan response
    // advReport->peerAddrType -> public/random

}


void ScanProgressEventHandler(CYBLE_GAPC_ADV_REPORT_T* param)
{

    uint8 i;
    uint8 adStructPtr = 0u;
    uint8 adTypePtr = 0u;
    uint8 nextPtr = 0u;
    uint16_t serviceUuid = 0;   
    
//    vLog(xBle, "advertising event type: %02x, rssi: %d, data len: %i,  data: ", param->eventType, param->rssi, param->dataLen);
//    for (int i = 0; i < param->dataLen; i++)
//        xprintf("%02x", param->data[i]);
//    xprintf(" peer addr type: %02x, bdaddr: ", param->peerAddrType);
//    for (int i = 0; i < sizeof(peerAddr.bdAddr); i++)
//        xprintf("%02x", param->peerBdAddr[i]);        
//    xprintf("\n");        

    /* Print and parse advertisement data and connect to compatible device */
    adStructPtr = 0u;
    for(i = 0; i < param->dataLen; i++)
    {
        if(i == adStructPtr)
        {
            adTypePtr = i + 1;
            adStructPtr += param->data[i] + 1;
            nextPtr = 1;
        }
        else if(i == (adTypePtr + nextPtr))
        {
            switch(param->data[adTypePtr])
            {
                case CYBLE_GAP_ADV_FLAGS:
                    break;

                case CYBLE_GAP_ADV_INCOMPL_16UUID:
                case CYBLE_GAP_ADV_COMPL_16UUID:
                    serviceUuid = CyBle_Get16ByPtr(&(param->data[i]));
//                    nextPtr += 2;
                    break;

                default:
                    break;
            }
        }
    }

    vLog(xBle, "found device: ");
    print_peer(param, serviceUuid);

    if (serviceUuid == bleMSRvServiceUuid)
    {
        peerDeviceFound = true;
        userStoppedScan = true;
        if (memcmp(peerAddr.bdAddr, param->peerBdAddr, sizeof(peerAddr.bdAddr)) == 0)
        {
            vLog(xBle, "same device!\n");
            if (authFailed > 2)
            {
                clearBanTs = GetTick() + BAN_SECONDS*1024;
                vLog(xBle, "too many login failures, banned for %i minutes!\n", BAN_SECONDS/60);
                return;
            }

        }
        else
        {
            authFailed = 0;
        }

        memcpy(peerAddr.bdAddr, param->peerBdAddr, sizeof(peerAddr.bdAddr));
        peerAddr.type = param->peerAddrType;
        vLog(xBle, "device with matching custom service discovered. stopping scan\n");
        CyBle_GapcStopScan();
    }
}


//void endUpload(void)
//{
//    uploadRetryTs = 0;
//    uploadRetryCount = 0;
//    uploading = false;
//    uploadedBytes = 0;
//    uploadCrc = 0;
//}


void uploadNextPacket(void)
{
    CYBLE_GATTS_HANDLE_VALUE_NTF_T  pkt;
    CYBLE_API_RESULT_T              r                           = CYBLE_ERROR_OK;    
    int                             len;
    header_t                        h;    
    uint8_t                         sendBuf[CYBLE_GATT_MTU-3];
    char                           *st                          = NULL;
    void                           *uploadPointer               = NULL;

    
    if (up.retry_ts != 0 && !TimerExpired(up.retry_ts))
        return;

    if (up.currentOffset >= dl->endOffset)
    {
        vLog(xDownload, "upload completed. reached last block requested\n");
        upload(false);
        return;
    }

    uploadPointer = flashSp() + up.currentOffset;
    assert(flashSp2() == flashSp());
    assert(flashSp() >= (void*)(CY_FLASH_BASE+MIN_FIRMWARE_SIZE));
    assert(MIN_FIRMWARE_SIZE > 32768);

    if (uploadPointer < flashSp2() ||
        uploadPointer < flashSp() ||
        uploadPointer < (void*)(CY_FLASH_BASE + MIN_FIRMWARE_SIZE)
    ) // must make sure is impossible to sending the firmware.
    {
        assert(0);
        vLog(xBUG, "requested upload out of boundaries!\n");
        upload(false);
        return;
    }
    
    if (uploadPointer >= flashEp() )
    {
        vLog(xDownload, "upload completed. reached end of memory. upload pointer: %lu\n", uploadPointer);
        upload(false);
        assert(uploadPointer < flashEp());    
        return;
    }
    assert(dl->endOffset <= flashEp()-flashSp());
    len = min(maxBlockSize(), dl->endOffset - up.currentOffset);
    h.offset = up.currentOffset;
    
    memcpy(sendBuf, &h, sizeof(header_t));
    memcpy(sendBuf + sizeof(header_t), uploadPointer, len);
    
    /* Create a new notification packet */
    pkt.attrHandle = CYBLE_MSRVXXX_DOWNLOAD_CHAR_HANDLE;
    pkt.value.val = sendBuf;
    pkt.value.len = len + sizeof(header_t);

#ifdef DEBUG_SEND_DELAY   
//    if (up.ok_ts == 0) 
//        uts = GetTick() - uts;
#endif    
        
//    st = "  indication";
    uint32_t ts = GetTick() + 10000;
    uint32_t nts = 0;
    do
    {
        if(TimerExpired(nts))
        {
            r = CyBle_GattsNotification(cyBle_connHandle, &pkt);
            if (r == CYBLE_ERROR_OK)
            {
                break;
            }
            nts = GetTick() + 5;
        }
        CyBle_ProcessEvents();
        wdtReset();
    } while (!TimerExpired(ts) && up.uploading);
    uint32_t delayed = GetTick() + 10000 - ts;
#ifdef DEBUG_SEND_DELAY
    udelay += delayed;
    pcnt++;
    if (delayed > 0)
    {
        ucnt++;    
        vLog(xDownload, "delayed %ims because ble stack was busy\n", delayed );
    }
#endif    
                   
    /* Report data to BLE component */
    if (r == CYBLE_ERROR_OK)
    {            
        uint32_t pts = 0;
        if (vLogFlags & (1<<xDownload))
        {
            if (up.last_ts != 0)
                pts = GetTick() - up.last_ts;
        }
        
        up.last_ts = GetTick();

        if (up.ok_ts == 0)        
        {
            up.ok_ts = GetTick();
            up.ok_count = 0;
            up.delay = 0;
        }
        else
        {
            up.ok_count++;
            up.delay = (delayed + GetTick()-up.ok_ts)/up.ok_count;
            
//            if (up.ok_count < 5 && up.delay > 10)
//                up.delay = 2;

//            if (delayed == 0)
//                up.delay = 0;
//            else
//                up.delay = min(10, (delayed + GetTick()-up.ok_ts)/up.ok_count);
            
#ifdef DEBUG_SEND_DELAY   
            if (vLogFlags & (1<<xDownload))
                vLog(xDownload, "result = %i, calculated delay = %i, interval: %i, packets: %i\n", r, up.delay, GetTick()-up.ok_ts, up.ok_count);
            else
                xprintf("%i+%i ", delayed, up.delay);
#endif            
        }        
                
//      uploadCrc = crc32(pkt.value.val, pkt.value.len, uploadCrc);
        vLog(xDownload, "sending %s: mem [%6lu..%6lu) aka file [%5lu..%5lu), %3u bytes, send in %2ims, next send in %2ims\n", st, uploadPointer, uploadPointer+len, up.currentOffset, up.currentOffset + len, len, pts, up.delay);
//      vLog(xBle, "cont_crc=0x%08x\n", uploadCrc);        

//        up.currentOffset += len+1;
        
        up.currentOffset += len;
        up.retry_ts = 0;
        up.retry_count = 0;
        up.bytes += len;        
        
    }
    else if (up.retry_count < 60 && r != CYBLE_ERROR_INVALID_PARAMETER)
    {
        up.retry_ts = GetTick() + 5;            
        up.retry_count++;
        
        vLog(xDownload, "sent failed. will retry in %i ms\n", up.retry_ts - GetTick());
//        indicationConfirmed = true;
    }
    else
    {
        vLog(xDownload, "upload aborted due to too many retries or r=%i\n", r);
        upload(false);
    }    
}

void validateDownloadParameters(download_t *dst, void *src)
{
    download_t tmp;
    
    memset(dst, 0, sizeof(download_t));
    
    memcpy(&tmp, src, sizeof(download_t));
    print_download(&tmp, "requested: ");
    
    uint32_t totalMem = flashEp()-flashSp();
    
    if (tmp.startOffset > tmp.endOffset)
    {
        vLog(xDownload, "dl size is negative. swapping values\n");
        uint32_t t = tmp.endOffset;
        tmp.endOffset = tmp.startOffset;
        tmp.startOffset = t;        
    }
    
    if (tmp.startOffset > totalMem)
    {
        vLog(xDownload, "dl start offset is out of memory.\n");
        tmp.startOffset = totalMem;
    }
        
    if (tmp.endOffset > totalMem)
    {
        vLog(xDownload, "endOffset was reduced from %i to %i because the we only have %i bytes of memory\n", tmp.endOffset, totalMem, totalMem);
        tmp.endOffset = totalMem;
    }
    
    memcpy(dst, &tmp, sizeof(download_t));
    print_download(dst, "accepted ");
}

    
void calc_crc_dl(download_t *d)
{
    uint32_t totalMem = flashEp()-flashSp();    
    uint32_t start = d->startOffset;
    uint32_t len = d->endOffset - d->startOffset;
    
    assert(start <= totalMem);
    assert(d->endOffset >= d->startOffset);
    
    if (start+len > totalMem)
    {
        len = totalMem-start;
        d->endOffset = totalMem;
    }
    
    uint32_t ts = GetTick();    
    d->crc = crc32(flashSp() + start, len, 0);
    ts = GetTick() - ts;
//    vLog(xDebug, "calculated crc in %ims from %i, len %i. crc = %08x\n", ts, start, len, d->crc);
        
}

void processWriteRequests(CYBLE_GATTS_WRITE_REQ_PARAM_T * param)
{
    bool                            ok           = true;
    CYBLE_GATT_ERR_CODE_T           errorCode;
    CYBLE_GATT_VALUE_T             *v;
    CYBLE_GATT_DB_ATTR_HANDLE_T	  	attr;
	
    v = &param->handleValPair.value;
    attr = param->handleValPair.attrHandle;
    vLog(xBle, "server sent command %s (%i)\n", cmd2st(attr), attr);

    switch(param->handleValPair.attrHandle)
    {
        case CYBLE_MSRVXXX_DEVICE_STATUS_CCCD_DESC_HANDLE:
//            vLog(xDownload, "write req: CYBLE_MSRVXXX_DEVICE_STATUS_CCCD_DESC_HANDLE\n");
            errorCode = CyBle_GattsWriteAttributeValue(&param->handleValPair, 0, &cyBle_connHandle, CYBLE_GATT_DB_PEER_INITIATED);            
            if (CYBLE_GATT_ERR_NONE == errorCode)
            {
                memcpy(&statsCccd, v->val, 2);
                vLog(xBle, "statsCccd: %04x\n", uploadCccd);
            }            
            else
            {
                vLog(xBle, "err=%i\n", errorCode); 
            }
            break;
        case CYBLE_MSRVXXX_DOWNLOAD_CCCD_DESC_HANDLE:
//            vLog(xDownload, "write req: CYBLE_MSRVXXX_DOWNLOAD_CCCD_DESC_HANDLE\n");
            errorCode = CyBle_GattsWriteAttributeValue(&param->handleValPair, 0, &cyBle_connHandle, CYBLE_GATT_DB_PEER_INITIATED);            
            if (CYBLE_GATT_ERR_NONE == errorCode)
            {                
                memcpy(&uploadCccd, v->val, 2);
                vLog(xDownload, "uploadCccd: %04x\n", uploadCccd);
                if (!up.uploading && uploadCccd != 0)
                    upload(true);
                else if (up.uploading && uploadCccd == 0)
                    upload(false);                
            }
            else
            {
                vLog(xDownload, "err=%i\n", errorCode);                
            }
            break;
        case CYBLE_MSRVXXX_DOWNLOAD_CHAR_HANDLE:
            {
//                vLog(xDownload, "write req: CYBLE_MSRVXXX_DOWNLOAD_CHAR_HANDLE\n");
                if (uploadCccd != 0)
                {
                    vLog(xDownload, "download declined! you should disable upload notifications first!\n");
                    break;
                }
                assert(up.uploading == false);
                if (v->len != sizeof(download_t))
				{
                    vLog(xDownload, "invalid param length for %s. Received: %i, expected: %i\n", cmd2st(attr), v->len, sizeof(download_t));
                    break;
				}
                                
                validateDownloadParameters(&stats.downloadParams, v->val);
                
                //print_download(dl, "curent   ");
                //print_download(&t,  "new      ");               
                if (stats.downloadParams.crc != 0xffffffff)
                {                    
                    calc_crc_dl(&stats.downloadParams);
                    print_download(&stats.downloadParams, "new w/crc");
                    sendStatsNotification();
                }
            }
            break;
        case CYBLE_MSRVXXX_SETTIME_CHAR_HANDLE: 
            setTime(v->val);
            break;
        case CYBLE_MSRVXXX_ERASE_CHAR_HANDLE:
            erase_pending = true;            
            break;            
        case CYBLE_MSRVXXX_UPGRADE_CHAR_HANDLE: 
            upgrade(v->val,v->len);
            break;
        case CYBLE_MSRVXXX_SETNAME_CHAR_HANDLE: 
            setName(v->val,v->len);
            break;
        case CYBLE_MSRVXXX_SETKEY_CHAR_HANDLE : 
            setKey(v->val,v->len);
            break;
        case CYBLE_MSRVXXX_SETPASS_CHAR_HANDLE: 
            setPass(v->val,v->len);
            break;
        case CYBLE_MSRVXXX_LOGIN_CHAR_HANDLE  :
            login(v->val,v->len);
            break;
        case CYBLE_MSRVXXX_REBOOT_CHAR_HANDLE  : 
            reboot();
            break;
        case CYBLE_MSRVXXX_LOGFLAGS_CHAR_HANDLE: 
            if (v->val[0] != FLAGS_PROTOCOL)
            {
                vLog(xBle, "Incompatible flags protocol: theirs=%i, ours=%i\n", v->val[0], FLAGS_PROTOCOL);
            }
            else
            {
                uint32_t old;
                old = vLogFlags;
                memcpy(&vLogFlags, &v->val[1], 4);
                compareLogFlags(old, vLogFlags);
            }
            
            break;
            
        default:
            ok = false;
            vLog(xBle, "command not handled: %s (%i)\n", cmd2st(attr), attr);
            break;
    }
    
    if (ok)
    {
        CYBLE_API_RESULT_T r;
        r = CyBle_GattsWriteRsp(cyBle_connHandle);
        if  (r != CYBLE_ERROR_OK && r != CYBLE_ERROR_INVALID_PARAMETER)
    	{
            vLog(xBle, "CyBle_GattsWriteRsp failed\n");
    		writeRspTs = GetTick() + 100;
    	}
    	else
    	{
            vLog(xBle, "CyBle_GattsWriteRsp()\n");
    		writeRspCnt = 0;
    	}
    }

}



void Ble_Stack_Handler( uint32 code, void *param)
{
    CYBLE_API_RESULT_T apiResult;
//    CYBLE_GATT_ERR_CODE_T           errorCode;

//    uint8 clientConfigDesc[2];
//    CYBLE_GATT_HANDLE_VALUE_PAIR_T  attrValue = {
//                                                    {(uint8 *)clientConfigDesc, 2, 2},
//                                                    CYBLE_MSRVXXX_DEVICE_STATUS_CLIENT_CHARACTERISTIC_CONFIGURATION_DESC_HANDLE
//                                                };

//    CYBLE_GAPC_ADV_REPORT_T		  *advReport;
#ifdef DEBUG_OUT
//    vLog(xBle, "--\n");
//    BleDebugOut(code, param);
//    vLog(xBle, "--\n");
#endif

    switch(code)
    {
        LOG_CASE(CYBLE_EVT_STACK_ON)
            apiResult = CyBle_GapGenerateLocalP256Keys();
            if(apiResult != CYBLE_ERROR_OK)
                vLog(xBle, "CyBle_GapGenerateLocalP256Keys API Error: %d\n", apiResult);

            CyBle_GapSetSecureConnectionsOnlyMode(1);
            if(apiResult != CYBLE_ERROR_OK)
                vLog(xBle, "CyBle_GapSetSecureConnectionsOnlyMode API Error: %d\n", apiResult);

            CyBle_GapcStartScan(CYBLE_SCANNING_FAST);
            vLog(xDebug, "started scan\n");
            break;

        case CYBLE_EVT_GAPC_SCAN_PROGRESS_RESULT:
            if (!peerDeviceFound)
                ScanProgressEventHandler(param);
//            else
//                vLog(xBle, "peer already found\n");

        break;

        LOG_CASE(CYBLE_EVT_TIMEOUT)
        LOG_CASE(CYBLE_EVT_GAPC_SCAN_START_STOP)
            vLog(xBle, "ble state: %i\n", CyBle_GetState());
            if(CyBle_GetState() == CYBLE_STATE_DISCONNECTED)
            {
                if(userStoppedScan == true)
                {
                    /* Scan stopped manually; do not restart scan */
                    userStoppedScan = false;
                    vLog(xBle, "connecting...\n");
                    apiResult = CyBle_GapcConnectDevice(&peerAddr);
                    if (CYBLE_ERROR_OK != apiResult)
                        vLog(xBle, "CyBle_GapcConnectDevice failed: %i\n", apiResult);

                }
                else
                {
                    /* Scanning timed out; Restart scan */
                    //UART_UartPutString("\n\nRestarting scan. ");
                    vLog(xBle, "restarted scanning...\n");
                    apiResult = CyBle_GapcStartScan(CYBLE_SCANNING_FAST);
                    if (CYBLE_ERROR_OK != apiResult)
                        vLog(xBle, "CyBle_GapcStartScan failed: %i\n", apiResult);


                }
            }
            break;


        LOG_CASE(CYBLE_EVT_GATT_CONNECT_IND        )
            apiResult = CyBle_GapGenerateLocalP256Keys();
            if(apiResult != CYBLE_ERROR_OK)
                vLog(xBle, "CyBle_GapGenerateLocalP256Keys API Error: %d \n", apiResult);
//            if(CyBle_GapSetOobData(cyBle_connHandle.bdHandle, CYBLE_GAP_OOB_ENABLE, securityKey, NULL, NULL)  != CYBLE_ERROR_OK)
//				vLog(xBle, "CyBle_GapSetOobData failed\n");
//            else
//                vLog(xBle, "oob set\n");
            break;
        LOG_CASE(CYBLE_EVT_GATTS_READ_CHAR_VAL_ACCESS_REQ)
            {
                CYBLE_GATTS_CHAR_VAL_READ_REQ_T *r = param;
                if (r->attrHandle == CYBLE_MSRVXXX_DEVICE_STATUS_CHAR_HANDLE)
                {
                    sendStatsNotification();
                }                
            }
            break;

        LOG_CASE(CYBLE_EVT_GATTC_READ_BLOB_RSP) break;
        LOG_CASE(CYBLE_EVT_STACK_BUSY_STATUS); break;


        LOG_CASE(CYBLE_EVT_GAP_DEVICE_CONNECTED)
            authFailed = 0;
            bleConnected = true;
            /* Once the devices are connected, the Client will not do a service
             * discovery and will assume that the handles of the Server are
             * known. This is because discovery of custom service is not a part
             * of the BLE component today and will be in the next upgrade to BLE
             * component.
             */

            /* Initiate an MTU exchange request */
            CyBle_GattcExchangeMtuReq(cyBle_connHandle, CYBLE_GATT_MTU);

            break;

        LOG_CASE(CYBLE_EVT_GAP_DEVICE_DISCONNECTED)
            resetAllFlags();
//            clientConfigDesc[0] = NOTIFICATON_DISABLED;
//            clientConfigDesc[1] = NOTIFICATON_DISABLED;

            vLog(xDebug, "started scan\n");
            CyBle_GapcStartScan(CYBLE_SCANNING_FAST);

            break;

        case CYBLE_EVT_GATTS_WRITE_CMD_REQ:

//            HandleUartRxTraffic((CYBLE_GATTS_WRITE_REQ_PARAM_T *) eventParam);

            break;

        case CYBLE_EVT_GATTS_XCNHG_MTU_REQ:

            //vLog(xBle, "CYBLE_EVT_GATTS_XCNHG_MTU_REQ\n");
            if(CYBLE_GATT_MTU > ((CYBLE_GATT_XCHG_MTU_PARAM_T *)param)->mtu)
            {
                mtuSize = ((CYBLE_GATT_XCHG_MTU_PARAM_T *)param)->mtu;
            }
            else
            {
                mtuSize = CYBLE_GATT_MTU;
            }
            vLog(xBle, "Mtu set to: %i\n", mtuSize);

            break;

        LOG_CASE (CYBLE_EVT_GATTS_WRITE_REQ)
                processWriteRequests(param);
                
			//vLog(xBle, "CyBle_GattsWriteRsp failed\n");
            //vLog(xBle, "CYBLE_EVT_GATTS_WRITE_REQ\n");
            /* This event is received when Central device sends a Write command
             * on an Attribute.
             * We first get the attribute handle from the event parameter and
             * then try to match that handle with an attribute in the database.
             */


            break;


        case CYBLE_EVT_GATTC_DISCOVERY_COMPLETE:
            vLog(xBle, "CYBLE_EVT_GATTC_DISCOVERY_COMPLETE\n");
            break;

        LOG_CASE(CYBLE_EVT_GAP_AUTH_REQ)
            //Request to Initiate Authentication received
            print_auth_info(xBle, &cyBle_authInfo, "my");
            print_auth_info(xBle, param, "peer's");
//            if (CyBle_GappAuthReqReply(cyBle_connHandle.bdHandle, &cyBle_authInfo)  != CYBLE_ERROR_OK)
//                vLog(xBle, "CyBle_GappAuthReqReply failed\n");
            if(CyBle_GapAuthReq(cyBle_connHandle.bdHandle, &cyBle_authInfo)  != CYBLE_ERROR_OK)
				vLog(xBle, "CyBle_GapAuthReq failed\n");
            break;

        case CYBLE_EVT_GAP_PASSKEY_DISPLAY_REQUEST:
            vLog(xBle, "CYBLE_EVT_GAP_PASSKEY_DISPLAY_REQUEST. Passkey is: %lu.\n", *(uint32 *)param);
            break;

        case CYBLE_EVT_GAP_AUTH_FAILED:
            vLog(xBle,"EVT_GAP_AUTH_FAILED, reason: ");
            switch(*(CYBLE_GAP_AUTH_FAILED_REASON_T *)param)
            {
                case CYBLE_GAP_AUTH_ERROR_CONFIRM_VALUE_NOT_MATCH:
                    xprintf("CONFIRM_VALUE_NOT_MATCH\n");
                    break;

                case CYBLE_GAP_AUTH_ERROR_INSUFFICIENT_ENCRYPTION_KEY_SIZE:
                    xprintf("INSUFFICIENT_ENCRYPTION_KEY_SIZE\n");
                    print_auth_info(xBle, &cyBle_authInfo, "my");
                    //vLog(xBle, "auth: %i\n", cyBle_authInfo.ekeySize);

                    fuckedup = true;
                    //CyBle_GapcStartScan(CYBLE_SCANNING_FAST);
                    break;

                case CYBLE_GAP_AUTH_ERROR_UNSPECIFIED_REASON:
                    xprintf("UNSPECIFIED_REASON\n");
                    break;

                case CYBLE_GAP_AUTH_ERROR_AUTHENTICATION_TIMEOUT:
                    xprintf("AUTHENTICATION_TIMEOUT\n");
                    break;

                default:
                    xprintf("0x%x  \n", *(CYBLE_GAP_AUTH_FAILED_REASON_T *)param);
                    break;
            }
            // Received when Authentication failed
            authFailed++;
            vLog(xBle, "authfailed++ = %i\n", authFailed);
            CyBle_GapDisconnect(cyBle_connHandle.bdHandle);
            break;

        LOG_CASE( CYBLE_EVT_GAP_KEYPRESS_NOTIFICATION ); break;
        LOG_CASE( CYBLE_EVT_GAP_OOB_GENERATED_NOTIFICATION ); break;
        LOG_CASE( CYBLE_EVT_GAP_PASSKEY_ENTRY_REQUEST ); break;
        LOG_CASE(CYBLE_EVT_GATTC_ERROR_RSP               ); break;
//        LOG_CASE(CYBLE_EVT_GATT_CONNECT_IND              ); break;
        LOG_CASE(CYBLE_EVT_GATT_DISCONNECT_IND           ); break;
        case CYBLE_EVT_GAP_ENCRYPT_CHANGE:
            vLog(xBle, "CYBLE_EVT_GAP_ENCRYPT_CHANGE: %x\n", *(uint8 *)param);
            break;
        LOG_CASE(CYBLE_EVT_GAP_KEYINFO_EXCHNGE_CMPLT   ); break;
        LOG_CASE(CYBLE_EVT_PENDING_FLASH_WRITE         ); break;
        LOG_CASE(CYBLE_EVT_LE_PING_AUTH_TIMEOUT        ); break;
        case CYBLE_EVT_HCI_STATUS:
            vLog(xBle, "EVT_HCI_STATUS: %x\n", *(uint8 *)param);

            break;

//        LOG_CASE(CYBLE_EVT_GATTS_XCNHG_MTU_REQ         ); break;
//        LOG_CASE(CYBLE_EVT_GATTS_READ_CHAR_VAL_ACCESS_REQ) break;
//        LOG_CASE(CYBLE_EVT_GATT_DISCONNECT_IND            ); break;
//        LOG_CASE(CYBLE_EVT_GATTS_XCNHG_MTU_REQ            ); break;
//        LOG_CASE(CYBLE_EVT_GATTS_WRITE_REQ                ); break;
//        LOG_CASE(CYBLE_EVT_GATTS_WRITE_CMD_REQ            ); break;
        LOG_CASE(CYBLE_EVT_GATTS_PREP_WRITE_REQ           ); break;
        LOG_CASE(CYBLE_EVT_GATTS_EXEC_WRITE_REQ           ); break;
//        LOG_CASE(CYBLE_EVT_GATTS_READ_CHAR_VAL_ACCESS_REQ ); break;
        LOG_CASE(CYBLE_EVT_GATTS_DATA_SIGNED_CMD_REQ      ); break;

        LOG_CASE(CYBLE_EVT_GATTC_XCHNG_MTU_RSP           ); break;
        LOG_CASE(CYBLE_EVT_GATTC_READ_BY_GROUP_TYPE_RSP  ); break;
        LOG_CASE(CYBLE_EVT_GATTC_READ_BY_TYPE_RSP        ); break;
        LOG_CASE(CYBLE_EVT_GATTC_FIND_INFO_RSP           ); break;
        LOG_CASE(CYBLE_EVT_GATTC_FIND_BY_TYPE_VALUE_RSP  ); break;
        LOG_CASE(CYBLE_EVT_GATTC_READ_RSP                ); break;
        LOG_CASE(CYBLE_EVT_GATTC_READ_MULTI_RSP          ); break;
        LOG_CASE(CYBLE_EVT_GATTC_WRITE_RSP               ); break;
        LOG_CASE(CYBLE_EVT_GATTC_EXEC_WRITE_RSP          ); break;
        LOG_CASE(CYBLE_EVT_GATTC_HANDLE_VALUE_NTF        ); break;
        LOG_CASE(CYBLE_EVT_GATTC_HANDLE_VALUE_IND        ); break;
        LOG_CASE(CYBLE_EVT_GATTC_STOP_CMD_COMPLETE       ); break;
    	LOG_CASE(CYBLE_EVT_GATTC_LONG_PROCEDURE_END      ); break;
//        LOG_CASE(CYBLE_EVT_GATTC_READ_BLOB_RSP) break;


        LOG_CASE(CYBLE_EVT_GATTS_HANDLE_VALUE_CNF)
//            indicationConfirmed = true;
        break;

        LOG_CASE( CYBLE_EVT_GAP_AUTH_COMPLETE );

                //todo
// example on how to store:
//              apiResult = CyBle_StoreBondingData(0u);
//              if ( apiResult == CYBLE_ERROR_OK)
//              {
//                  printf("Bonding data stored\n");
//              }
//              else
//              {
//                  printf ("Bonding data storing pending\n");
//              }

// example on how to clear bonding
//              CYBLE_GAP_BD_ADDR_T clearAllDevices = {{0,0,0,0,0,0},0};
//            apiResult = CyBle_GapRemoveDeviceFromWhiteList(&clearAllDevices);
//            /* CyBle_StoreBondingData should be called to clear the Bonding info
//                from flash */
//            while(CYBLE_ERROR_OK != CyBle_StoreBondingData(1));
//            printf("Cleared the list of bonded devices. \n\n");


// also read about CyBle_GapRemoveBondedDevice()


            break;

        LOG_CASE(CYBLE_EVT_GAP_SMP_NEGOTIATED_AUTH_INFO)
            peerAuthInfo = *(CYBLE_GAP_AUTH_INFO_T *)param;
            print_auth_info(xBle, &cyBle_authInfo, "my");
            print_auth_info(xBle, &peerAuthInfo,"negotiated");
            break;



        case CYBLE_EVT_GAP_NUMERIC_COMPARISON_REQUEST:
//            vLog(xBle, "Compare this passkey with displayed in your peer device and press 'y' or 'n': %6ld \n", *(uint32 *)param);
            vLog(xBle, "peer provided pass: %6ld \n", *(uint32 *)param);
            apiResult = CyBle_GapAuthPassKeyReply(cyBle_connHandle.bdHandle, 0u, CYBLE_GAP_ACCEPT_PASSKEY_REQ);
            if(apiResult != CYBLE_ERROR_OK)
            {
                vLog(xBle, "CyBle_GapAuthPassKeyReply API Error: %d \n", apiResult);
            }

            break;

        default:
            vLog(xBle, "unknown case: %i (0x%x)\n", code, code);
            break;


    }

}

void sendStatsNotification(void)
{
    /* Create a new notification packet */
    CYBLE_GATTS_HANDLE_VALUE_NTF_T notificationPacket;

    /* Update Notification packet with the data.
     * Maximum data which can be sent is equal to (Negotiated MTU - 3) bytes.
     */
    stats.timestamp = timestamp;
    notificationPacket.attrHandle = CYBLE_MSRVXXX_DEVICE_STATUS_CHAR_HANDLE;
    notificationPacket.value.val = (void*)&stats;
    notificationPacket.value.len = sizeof(stats);

    /* Report data to BLE component */
    CyBle_GattsNotification(cyBle_connHandle, &notificationPacket);
//    vLog(xBle, "sent stats notification\n");
    showStats((stats.downloadParams.crc == 0xffffffff) ? xDownload : xInfo, &stats);
}

//void SendData(int offset, int size)
//{
//    /* Create a new notification packet */
//    CYBLE_GATTS_HANDLE_VALUE_NTF_T notificationPacket;
//
//    /* Update Notification packet with the data.
//     * Maximum data which can be sent is equal to (Negotiated MTU - 3) bytes.
//     */
//    notificationPacket.attrHandle = CYBLE_MSRVXXX_DEVICE_STATUS_CLIENT_CHARACTERISTIC_CONFIGURATION_DESC_HANDLE;
//    notificationPacket.value.val = stats;
//    notificationPacket.value.len = sizeof(stats);
//
//    /* Report data to BLE component */
//    CyBle_GattsNotification(cyBle_connHandle, &notificationPacket);
//}


void HandleBleProcessing(void)
{
    CYBLE_API_RESULT_T apiResult;

    if (fuckedup)
    {
        fuckedup = false;
        CyBle_Stop();
        resetAllFlags();
        apiResult = CyBle_Start(Ble_Stack_Handler);
        if(apiResult != CYBLE_ERROR_OK)
            vLog(xBle, "CyBle_Start Error: %d\n", apiResult);
        assert(CYBLE_ERROR_OK == apiResult);

//
//        CyBle_SoftReset();
//        if (CyBle_GetState() != CYBLE_STATE_SCANNING)
//            CyBle_GapcStartScan(CYBLE_SCANNING_FAST);
    }
    
    if (erase_pending)
    {
        flashErase();
        erase_pending = false;
    }
    if (clearBanTs != 0 && TimerExpired(clearBanTs))
    {
        if(CyBle_GetState() != CYBLE_STATE_CONNECTED)
        {
            vLog(xDebug, "Unbanned\n");
            authFailed = 0;
            peerDeviceFound = false;
            userStoppedScan = false;
            if (CyBle_GetState() != CYBLE_STATE_SCANNING)
            {
                vLog(xDebug, "started scan\n");
                CyBle_GapcStartScan(CYBLE_SCANNING_FAST);
            }
            clearBanTs = 0;
        }
        else
        {
            vLog(xDebug, "state: %i\n", CyBle_GetState());
        }
    }

    if (!bleConnected)
        return;

    if (CyBle_GattGetBusyStatus() != CYBLE_STACK_STATE_FREE)
        vLog(xBle, "busy check works\n");

//    if(up.uploading && indicationConfirmed && CyBle_GattGetBusyStatus() == CYBLE_STACK_STATE_FREE)
    if(up.uploading && CyBle_GattGetBusyStatus() == CYBLE_STACK_STATE_FREE)
    {
        if (up.delay != 0 && up.last_ts != 0)
        {
            if (TimerExpired(up.last_ts+up.delay))
            {
//                vLog(xDownload, "last: %lu, delay: %lu, now: %lu, diff: %lu\n", up.last_ts, up.delay, GetTick(), GetTick() - up.last_ts);
                uploadNextPacket();
            }
        }
        else
        {
            uploadNextPacket();
        }
    }

    if (writeRspTs != 0 && TimerExpired(writeRspTs))
    {
        if (writeRspCnt < 50 && CyBle_GattsWriteRsp(cyBle_connHandle) != CYBLE_ERROR_OK)
        {            
            writeRspTs = GetTick() + 10;
            writeRspCnt++;
        }
        else
        {
            vLog(xBle, "CyBle_GattsWriteRsp failed !!!\n");
            writeRspTs = 0;
            writeRspCnt = 0;
        }
    }

}

/* [] END OF FILE */
