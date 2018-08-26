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


#include "bluetooth.h"
#include "setjmp.h"

#define MAX_RETRY (9)
#define MAX_BAD_PKT (2)
#define MAX_BLE_FAILURES (5)
#define DEFAULT_MTU_SIZE            (23)
#define LOG_CASE(__A) case __A: vLog(xBle, __stringnify(__A) "\n");


volatile uint8_t        statsNotify;
volatile bool           bleConnected            = false;
volatile bool           connectionReady         = false;
volatile bool           userStoppedScan         = false;
volatile int            ble_fail                = 0;
static jmp_buf          exception_env;
bool                    end_dl                  = false;   
uint16_t                downloadCccd            = 0;
//ble_event_t             ble_events              = None;
uint8_t                 lock_download           = 0;
int8_t                  lock_yield              = 0;
volatile int            unconfirmed_writes      = 0;
volatile int            unconfirmed_reads       = 0;
volatile int            incomplete_download     = 0;
int                     pending_stats_read      = 0;
uint32_t                last_ts                 = 0;
uint32_t                log_sync                = 1;

typedef struct 
{
    uint32_t start_ts; // timestamp when we started the download - keep
    uint32_t last_ts; 
    volatile uint32_t timeout_ts; // timestamp when we got the first bad package
    uint32_t wrong_pkt_ts; // timestamp when we got the first bad package
    uint8_t  wrong_pkt_count;
    uint8_t  retry_count; 
    
    uint32_t bytes_received;
    uint32_t startOffset;
    uint32_t endOffset;
    uint32_t currentOffset;
    uint32_t crc;   
} download_handler_t;

uint32_t dlLen(download_handler_t *dlh)
{
    return dlh->endOffset - dlh->startOffset;
}


download_handler_t dlh;
download_t * dlp = &stats.downloadParams;

CYBLE_API_RESULT_T apiResult;

CYBLE_GAP_AUTH_INFO_T peerAuthInfo;
//uint8 securityKey[] = "1234567890123456";

/* MTU size to be used by Client and Server after MTU exchange */
uint16 mtuSize      = CYBLE_GATT_MTU;   

CYBLE_GAP_BD_ADDR_T             peerAddr;           /* BD address of the peer device */
CYBLE_CONN_HANDLE_T             connHandle;

void resetAllVars(void)
{
    memset(&dlh, 0, sizeof(dlh));
    memset(&dlp, 0, sizeof(dlp));
    bleConnected = false;
    connectionReady = false;
    unconfirmed_writes = 0;
    unconfirmed_reads = 0;
    ble_fail = 0;    
}

uint16_t maxBlockSize(void)
{
    return (mtuSize - 3 - sizeof(header_t)) / 4 * 4;
}

bool isDownloading(void)
{
//    vLog(xBle, "is downloading: %i\n", lock_download);
    return lock_download != 0;
}


CYBLE_API_RESULT_T writeCharacteristicDescriptors(CYBLE_CONN_HANDLE_T connHandle, CYBLE_GATTC_WRITE_REQ_T * writeReqParam)
{
    CYBLE_API_RESULT_T      result = CYBLE_ERROR_OK;
    uint32_t ts = GetTick() + 1000;
    uint32_t nts = GetTick()-1;
    do
    {
        if(TimerExpired(nts))
        {
            result = CyBle_GattcWriteCharacteristicDescriptors(connHandle, writeReqParam);
            if (result == CYBLE_ERROR_OK)
            {
                break;
            }
            nts = GetTick() + 2;
        }
        CyBle_ProcessEvents();
        wdtReset();
    } while (!TimerExpired(ts));                       
    uint32_t delayed = GetTick() + 1000 - ts;    
    if (delayed > 0)
        vLog(xDownload, "delayed %ims because ble stack was busy\n", delayed );
        
    return result;
}

CYBLE_API_RESULT_T readCharacteristicDescriptors(CYBLE_CONN_HANDLE_T connHandle, CYBLE_GATTC_READ_REQ_T				 readReqParam)
{
    CYBLE_API_RESULT_T      result = CYBLE_ERROR_OK;
    uint32_t ts = GetTick() + 1000;
    uint32_t nts = GetTick()-1;
    do
    {
        if(TimerExpired(nts))
        {
            result = CyBle_GattcReadCharacteristicDescriptors(connHandle, readReqParam);
            if (result == CYBLE_ERROR_OK)
            {
                break;
            }
            nts = GetTick() + 2;
        }
        CyBle_ProcessEvents();
        wdtReset();
    } while (!TimerExpired(ts));                       
    uint32_t delayed = GetTick() + 1000 - ts;    
    if (delayed > 0)
        vLog(xDownload, "delayed %ims because ble stack was busy\n", delayed );
        
    return result;
}


bool sendCommand(CYBLE_GATT_DB_ATTR_HANDLE_T command, void *args, uint8 len)
{
    CYBLE_GATTC_WRITE_REQ_T request;
    CYBLE_API_RESULT_T      result;
    
    request.attrHandle = command;    
    request.value.len = len;
    request.value.val = args;
    result = writeCharacteristicDescriptors(cyBle_connHandle, &request); 
    
    if (CYBLE_ERROR_OK != result)
    {
        vLog(xBle, "Failed to send command %s. Reason: %i, ble_fail: %i\n", cmd2st(command), result, ble_fail);
        ble_fail++;
        return false;
    }
    else
    {
        unconfirmed_writes++;
        ble_fail = 0;                
#ifdef DEBUG        
        if (vLogFlags & (1<<xBle))
        {
            vLog(xBle, "%s was written in server ", cmd2st(command));
            if (len > 0)
            {
                xprintf("(args len: %i, ", len);
                
                bool isAscii = true;
                
                for (int i = 0; i < len; i++)
                {
                    uint8_t ch = ((uint8_t *)args)[i];
                    if (( ch < 32 || ch >= 127) && (i != len-1 || ch != 0 ))
                        isAscii = false;
                }
                    
                if (isAscii)
                {
                    myputchar('"');
                    for (int i = 0; i < len; i++)
                        myputchar(((uint8_t *)args)[i]);
                    myputchar('"');
                }
                else
                {
                    for (int i = 0; i < len; i++)
                        xprintf("%02x", ((uint8_t *)args)[i]);
                }
                myputchar(')');
                myputchar('\n');
//              xprintf("). unconfirmed_writes: %i\n", unconfirmed_writes);
            }
            else
            {
                xprintf("(no arguments)\n");
            }
                    
        }
#endif        
        
                        
//        vLog(xBle, "%sd notifications (value: %i), unconfirmed_writes: %i\n", st, flags, unconfirmed_writes);
    }
    
    return true;    
}


bool setDownloadNotifications(bool enabled)
{
	uint16_t                flags;
    bool                    result;
//    uint8_t                 rc;
    char const             *st;
        
    if (enabled)
    {
        st = "Enable";
        flags = (1<<8) | 1;
    }
    else
    {
        st = "Disable";
        flags = 0;                  // disable notifications and indications
    }
        
    result = sendCommand(CYBLE_MSRVXXX_DOWNLOAD_CCCD_DESC_HANDLE, &flags, 2);
        
    if (result)
    {
        downloadCccd = flags;
    }
    else
    {
        vLog(xDownload, "Failed to %s notifiactions. Reason: %i, ble_fail: %i\n", st, result, ble_fail);
    }
    return result;
}

void processReadResponse(CYBLE_GATTC_READ_RSP_PARAM_T *v)
{             
    if (unconfirmed_reads > 0)
        unconfirmed_reads--;
//    vLog(xBle, "received %i bytes\n", v->value.len);
    if (v->value.len == sizeof(download_t) && lock_download)
    {
//        vLog(xBle, "valid download Read\n", v->value.len);
        memcpy(&dlp, v->value.val, sizeof(dlp));        
        print_download(dlp, "params accepted by server");
        //ble_events |= ReadRsp;
    }
    else if (v->value.len > 0)
    {
        vLog(xBle, "unknown read response from server. size: %i\n", v->value.len);        
    }
//    vLog(xDebug, "unconfirmed writes %i, unconfirmed_reads %i, incomplete dl %i\n", unconfirmed_writes, unconfirmed_reads, incomplete_download);
    
}


bool yield(uint32_t delay, volatile int *semaphore)
{
//    vLog(xDownload, "lock_yield: %i\n", lock_yield);
    assert(lock_yield == 0);
    lock_yield++;
//    ble_events = None;
    uint32_t ts = GetTick() + delay;   
    int prev_fail = ble_fail;
    int prev_semaphore = *semaphore;
    bool ret = true;
    
//    vLog(xDebug, "unconfirmed writes %i, unconfirmed_reads %i, incomplete dl %i\n", unconfirmed_writes, unconfirmed_reads, incomplete_download);
//    if (prev_semaphore == 0)
//        vLog(xDownload, "warning: the passed in semaphore is already 0\n");
    
    while(!TimerExpired(ts) && ble_fail >= prev_fail)
    {
        ret = false;
        if (*semaphore == 0 || *semaphore == prev_semaphore-1)
        {
//            vLog(xDownload, "semaphore confirmed. exiting after %i ms\n", GetTick() + delay - ts);
            ret = true;
            break;
        }
        if (lock_download > 0 && (end_dl || ble_fail >= MAX_BLE_FAILURES))
        {
            vLog(xDownload, "jumping... end_dl: %i, ble_fail: %i, lock_yield: %i, lock_download: %i\n", end_dl, ble_fail, lock_yield, lock_download);
            lock_yield=0;
            longjmp(exception_env,1); // exit immediatelly
        }
        main_loop();
    }
    
    lock_yield--;
    return ret;
//    vLog(xDownload, "lock_yield: %i\n", lock_yield);   
}

bool writeDownloadParams(download_t *d)
{

    print_download(d, "requested ");        
    if (d->endOffset <= d->startOffset)
    {
        vLog(xDownload, "requested zero or negative length\n");
        return false;
    }
    
    return sendCommand(CYBLE_MSRVXXX_DOWNLOAD_CHAR_HANDLE, d, sizeof(download_t));       
}

bool writeLogFlags(uint32_t flags)
{
    char buf[5];
    buf[0] = FLAGS_PROTOCOL;
    memcpy(&buf[1], &flags, sizeof(uint32_t));
    
    return sendCommand(CYBLE_MSRVXXX_LOGFLAGS_CHAR_HANDLE, buf, sizeof(buf));       
}


//bool readDownloadParams(void)
//{
//    return readParams(CYBLE_MSRVXXX_DOWNLOAD_CHAR_HANDLE);
//}

bool readStats(void)
{
    if (!connectionReady)
    {
        vLog(xDebug, "not connected\n");
        return false;
    }
    
    if (readParams(CYBLE_MSRVXXX_DEVICE_STATUS_CHAR_HANDLE))
    {
        pending_stats_read = 1;
        return true;
    }
    else
    {
        return false;
    }
}

bool readParams(CYBLE_GATTC_READ_REQ_T handle)
{
    CYBLE_API_RESULT_T      result;
               
    result = readCharacteristicDescriptors(cyBle_connHandle, handle);
    if (CYBLE_ERROR_OK != result)
    {
        ble_fail++;        
        vLog(xDownload, "failed to read characteristic descriptor %i. api error: %i, ble_fail: %i\n", handle, result, ble_fail);
        return false;
    }
    else
    {
        unconfirmed_reads++;
//        vLog(xDownload, "unconfirmed_reads: %i\n", unconfirmed_reads);
        ble_fail=0;
    }

    vLog(xDownload, "read request for %i has been sent to server\n", handle);    
    return true;
}

bool stopDownload(char *reason)
{
    
    end_dl = true;
    if (downloadCccd == 0)
        return true;
//    vLog(xDownload, "Unsubscribing from download packets\n");            
    if (setDownloadNotifications(false))
    {    
        assert(GetTick() >= dlh.start_ts);
        uint32_t ll = xInfo;
        if (dlh.bytes_received != dlp->endOffset - dlp->startOffset)
            ll = xDownload;
        
        vLog(ll, "Download %s (%lu bytes, %i.%ikb/sec, %lums) ", 
            reason,
            dlh.bytes_received, 
            dlh.bytes_received / (GetTick() - dlh.start_ts), 
            (10 * dlh.bytes_received / (GetTick() - dlh.start_ts))%10, 
            (GetTick() - dlh.start_ts) * 1000 / 1024);                    

        if (vLogFlags & (1<<ll))
        {
            if (dlh.bytes_received == dlp->endOffset - dlp->startOffset && dlh.crc != dlp->crc)
                xprintf("our ");
                
            xprintf("crc: 0x%08x, ", dlh.crc);
            
            if (dlh.bytes_received != dlp->endOffset - dlp->startOffset)
                xprintf("download incomplete\n");
            else if(dlh.crc == dlp->crc)
                xprintf("CRC ok\n");
            else
                xprintf("their crc: 0x%08x CRC FAILED!\n", dlp->crc);
        }
        
        return true;
    }
    else
    {
        vLog(xDownload, "Stop download FAILED.\n");
        return false;
    }
}

void processDownloadPkt(uint8_t *buf, uint16_t len)
{
    header_t h;
    
       
    if (lock_download == 0)
    {
        stopDownload("I do not expect any download packages");        
        return;
    }

    if (len <= sizeof(header_t))
    {
        vLog(xDownload, "invalid length of download packet: %i\n", len);
        return;
    }
   
    memcpy(&h, buf, sizeof(header_t));
    
    if (h.offset != dlh.currentOffset)
    {
        dlh.wrong_pkt_count++;
        vLog(xDownload, "wrong packet (count:%i). received pkt id %i, expected id: %i\n", dlh.wrong_pkt_count, h.offset, dlh.currentOffset);
        if (dlh.wrong_pkt_ts == 0)
            dlh.wrong_pkt_ts = GetTick() + 1024;                                                
        return;
    }
    
    // correct packet
    dlh.wrong_pkt_ts = 0;
    dlh.wrong_pkt_count = 0;
    dlh.retry_count = 0;
                                
    len -= sizeof(header_t);
    assert(dlh.endOffset > dlh.startOffset);
    uint32_t remaining = dlh.endOffset - dlh.currentOffset;    
    if (len > remaining)
    {
        vLog(xDebug, "too much data received (%i). expected max %i. dropping %i bytes\n", len, remaining, len - remaining);
        len = remaining;
    }
    
    uint32_t new_crc = crc32(buf + sizeof(header_t), len, dlh.crc);
//    vLog(xDebug, "old crc: %08x, new crc: %08x\n", dlh.crc, new_crc);
    dlh.crc = new_crc;
    
    uint32_t pts;
    if (last_ts != 0)
        pts = GetTick() - last_ts;
    else
        pts = 0;
        
    last_ts = GetTick();
                                
//    vLog(xDownload, "recv block %i len %i pkt_crc=0x%08x cont_crc=0x%08x\n", dlh.pkt_id, len, mycrc, dlh.crc);
    vLog(xDownload, "recv data file [%5lu..%5lu) cont_crc=0x%08x, recv in %ims\n", 
        dlh.currentOffset,
        dlh.currentOffset+len,
        dlh.crc,
        pts);
                                
    dlh.currentOffset += len;
    dlh.bytes_received += len;
                                
    if (dlh.currentOffset == dlh.endOffset)
    {
        if (incomplete_download > 0)
            incomplete_download--;
        assert(incomplete_download == 0);
    }
    else
    {
        dlh.timeout_ts = GetTick() + 500;
    }
    
}

bool retryDownload()
{
    dlh.retry_count++;
    if (dlh.retry_count > MAX_RETRY ||  dlh.currentOffset >= dlh.endOffset)
    {            
        vLog(xDownload, "retryDownload() decided to cancel the download. retry_count: %i, dlh.currentOffset: %i, dlh.endOffset: %i\n", dlh.retry_count, dlh.currentOffset, dlh.endOffset);
        stopDownload("too many retries");
        return false;
    }
    else if (stopDownload("canceled")) 
    {
        end_dl = false; // don't let yield to long jump
        vLog(xDownload, "Retrying %i/%i...\n", dlh.retry_count, MAX_RETRY);
        
        if (!yield(500, &unconfirmed_writes))
        {
            vLog(xDownload, "failed to stop the current download\n");            
            return false;
        }
                           
        download_t nd;
        nd.startOffset = dlh.currentOffset;
        nd.endOffset = dlh.endOffset;
        nd.crc = 0xffffffff;
        
        if (!writeDownloadParams(&nd))
        {
            vLog(xDownload, "failed to write download params\n");
            return false;
        }
        if (!yield(500, &unconfirmed_writes))
        {
            vLog(xDownload, "failed to write download params\n");
            return false;
        }

        setDownloadNotifications(true);
        if (!yield(500, &unconfirmed_writes))
        {
            stopDownload("failed to enable notifications");
            return false;
        }
    }
    
    return true;
}


void startDownload(download_t *d)
{    
    if (d->endOffset <= d->startOffset)
        return;

    if (!writeDownloadParams(d))
    {
        return;
    }
    pending_stats_read = 1;
    if (!yield(500, &pending_stats_read))
    {
        vLog(xDebug, "!!! timeout waiting for stats. unconfirmed_writes = %i, pending_stats_read = %i\n", unconfirmed_writes, pending_stats_read);
        return;
    }
    
        
//    print_download(dlp, "params accepted by server");
    dlh.start_ts = GetTick();
    dlh.startOffset = dlp->startOffset;
    dlh.currentOffset = dlp->startOffset;
    dlh.endOffset = dlp->endOffset;
    dlh.crc = 0;
    dlh.bytes_received = 0;
    dlh.wrong_pkt_count = 0;
    dlh.wrong_pkt_ts = 0;
    dlh.retry_count = 0;    
    
    if (dlp->startOffset >= dlp->endOffset)
    {
        vLog(xDebug, "downloaded aborted. nothing to download from %i to %i\n", dlp->startOffset, dlp->endOffset);
        return;
    }
    
    setDownloadNotifications(true); // enable download
    if (!yield(500, &unconfirmed_writes))
    {
        stopDownload("failed to enable notifications");
        return;
    }
    
    dlh.timeout_ts = GetTick() + 500;
    dlh.start_ts = GetTick();
//    vLog(xDownload, "start download benchmark...\n");
    do {
                       
        if (!yield(10, &incomplete_download))
        {
            if (dlh.timeout_ts != 0 && TimerExpired(dlh.timeout_ts))
            {
                dlh.timeout_ts = 0;
                vLog(xDownload, "download timed out...\n");                    
                if (!retryDownload())
                {
                    break;
                }
            } 
            else if (dlh.wrong_pkt_count >= MAX_BAD_PKT || (dlh.wrong_pkt_ts != 0 && TimerExpired(dlh.wrong_pkt_ts)))
            {   
                vLog(xDownload, "wrong packets received count: %i, ts: %lu, expired: %i...\n", dlh.wrong_pkt_count, dlh.wrong_pkt_ts, TimerExpired(dlh.wrong_pkt_ts));
                dlh.wrong_pkt_ts = 0;
                dlh.wrong_pkt_count = 0;
                
                if (!retryDownload())
                {
                    break;
                }
            }            
        } 
        else
        {
            stopDownload("complete");
            break;
        } 
    } while (1);
}

void cancelDownload(char *st)
{
    if (!connectionReady)
    {
        vLog(xDebug, "not connected\n");
        return;
    }

    stopDownload(st);
    if (lock_download > 0)
    {
        vLog(xDownload, "jumping... end_dl: %i, ble_fail: %i, lock_yield: %i, lock_download: %i\n", end_dl, ble_fail, lock_yield, lock_download);
        longjmp(exception_env,1);
    }
    vLog(xDownload, "no jump\n");
}

uint32_t download(uint32_t start, uint32_t end)
{   
    if (end == start)
        return 0;
    
    if (end < start)
    {        
        vLog(xBUG, "requested negative length\n");
        return 0;
    }
    if (!connectionReady)
    {
        vLog(xDebug, "not connected\n");
        return 0;
    }
    
//    vLog(xDebug, "lock_download: %i, lock_yield: %i\n", lock_download, lock_yield);        
    if (lock_download > 0)
        cancelDownload("cancelled by user");
    
    lock_download++;
        
    if (setjmp(exception_env) == 0)
    {
        end_dl = false;
        incomplete_download = 1;
        
        download_t dl;        
        dl.startOffset = start;
        dl.endOffset = end;
        dl.crc = 0;
        last_ts = GetTick();
        
        startDownload(&dl);
    }

    lock_download--;
    lock_yield = 0;
//    vLog(xDebug, "lock_download: %i, lock_yield: %i\n", lock_download, lock_yield);
    assert(lock_download == 0);
    return dlh.bytes_received;
}


void Ble_Stack_Handler(uint32 eventCode, void * eventParam)
{

    switch(eventCode)
    {
        /* Stack initialized; ready for advertisement */
        case CYBLE_EVT_STACK_ON:
            
            apiResult = CyBle_GapGenerateLocalP256Keys();
            if(apiResult != CYBLE_ERROR_OK)
                vLog(xBle, "CyBle_GapGenerateLocalP256Keys API Error: %d \r\n", apiResult);
                
            CyBle_GapSetSecureConnectionsOnlyMode(1);
            if(apiResult != CYBLE_ERROR_OK)
                vLog(xBle, "CyBle_GapSetSecureConnectionsOnlyMode API Error: %d \r\n", apiResult);
                                        
            vLog(xBle, "Advertising with Address: ");            
            
            print_mac(xBle, &cyBle_deviceAddress, *(uint16_t*)cyBle_customCServ[0].uuid);
            CyBle_GappStartAdvertisement(CYBLE_ADVERTISING_FAST);
            break;
        
        /* Advertisement timed out; Restart advertisement */
        case CYBLE_EVT_GAPP_ADVERTISEMENT_START_STOP:
            if(CyBle_GetState() == CYBLE_STATE_DISCONNECTED)
            {
                CyBle_GappStartAdvertisement(CYBLE_ADVERTISING_FAST);
            }
            break;

        case CYBLE_EVT_GAP_DEVICE_CONNECTED:
            
//            if(CyBle_GapSetOobData(cyBle_connHandle.bdHandle, CYBLE_GAP_OOB_ENABLE, securityKey, NULL, NULL)  != CYBLE_ERROR_OK)
//				vLog(xBle, "CyBle_GapSetOobData failed\n");
//            else
//                vLog(xBle, "oob set\n");        
                
                
            vLog(xBle, "requesting secure channel...\n");
            print_auth_info(xBle, &cyBle_authInfo, "my");
            if(CyBle_GapAuthReq(cyBle_connHandle.bdHandle, &cyBle_authInfo)  != CYBLE_ERROR_OK)
				vLog(xBle, "CyBle_GapAuthReq failed\n");
            
            vLog(xBle, "Connected.\n");
            CyBle_GattcExchangeMtuReq(cyBle_connHandle, CYBLE_GATT_MTU);            
            bleConnected = true;
            
            break;
            
        LOG_CASE(CYBLE_EVT_GATT_CONNECT_IND        );
            break;
                        
        /* Device disconnected; restart advertisement */
        case CYBLE_EVT_GAP_DEVICE_DISCONNECTED:
            bleConnected = false;
            connectionReady = false;

            vLog(xBle, "Disconnected.\n");
            vLog(xBle, "Advertising again with ");
            print_mac(xBle, &cyBle_deviceAddress, *(uint16_t*)cyBle_customCServ[0].uuid);
            
            mtuSize = DEFAULT_MTU_SIZE;
            vLog(xBle, "Mtu requested: %i\n", mtuSize);                            
            
            CyBle_GappStartAdvertisement(CYBLE_ADVERTISING_FAST);
            break;

           
            
        case CYBLE_EVT_GATTC_XCHNG_MTU_RSP:
            {                
                CYBLE_GATT_XCHG_MTU_PARAM_T * mp = eventParam;
                mtuSize = mp->mtu;
                vLog(xBle, "Mtu agreed to: %i\n", mp->mtu);                            
                CyBle_GappAuthReqReply(cyBle_connHandle.bdHandle, &cyBle_authInfo);
                break;                
                
            }
            break;                 
            
        case CYBLE_EVT_GATTC_DISCOVERY_COMPLETE:
            vLog(xBle, "CYBLE_EVT_GATTC_DISCOVERY_COMPLETE\n");
            break;
            
        LOG_CASE(CYBLE_EVT_GAP_AUTH_REQ)
                print_auth_info(xBle, &cyBle_authInfo, "my");
                print_auth_info(xBle, eventParam, "peer's");
                //Request to Initiate Authentication received
                CyBle_GappAuthReqReply(cyBle_connHandle.bdHandle, &cyBle_authInfo);
            break;
            
        case CYBLE_EVT_GAP_PASSKEY_DISPLAY_REQUEST:
            vLog(xBle, "CYBLE_EVT_GAP_PASSKEY_DISPLAY_REQUEST. Passkey is: %lu.\r\n", *(uint32 *)eventParam);
            //vLog(xBle, "Please enter the passkey on your Server device.\r\n");
            break;
           
        case CYBLE_EVT_GAP_AUTH_FAILED:
            vLog(xBle,"EVT_GAP_AUTH_FAILED, reason: ");
            switch(*(CYBLE_GAP_AUTH_FAILED_REASON_T *)eventParam)
            {
                case CYBLE_GAP_AUTH_ERROR_CONFIRM_VALUE_NOT_MATCH:
                    xprintf("CONFIRM_VALUE_NOT_MATCH\n");
                    break;
                    
                case CYBLE_GAP_AUTH_ERROR_INSUFFICIENT_ENCRYPTION_KEY_SIZE:
                    xprintf("INSUFFICIENT_ENCRYPTION_KEY_SIZE\n");
                    break;
                
                case CYBLE_GAP_AUTH_ERROR_UNSPECIFIED_REASON:
                    xprintf("UNSPECIFIED_REASON\n");
                    break;
                    
                case CYBLE_GAP_AUTH_ERROR_AUTHENTICATION_TIMEOUT:
                    xprintf("AUTHENTICATION_TIMEOUT\n");
                    break;
                    
                default:
                    xprintf("0x%x  \n", *(CYBLE_GAP_AUTH_FAILED_REASON_T *)eventParam);
                    break;
            }
            break;
            
            
    
       LOG_CASE(CYBLE_EVT_GAP_SMP_NEGOTIATED_AUTH_INFO)
            peerAuthInfo = *(CYBLE_GAP_AUTH_INFO_T *)eventParam;
            print_auth_info(xBle, &peerAuthInfo,"peer's");
            print_auth_info(xBle, &cyBle_authInfo, "my");
            break;
  
        LOG_CASE(CYBLE_EVT_GAP_AUTH_COMPLETE           )
            connectionReady = true;
            if (log_sync)
                writeLogFlags(vLogFlags);            
            break;
                               
    
      case CYBLE_EVT_GAP_NUMERIC_COMPARISON_REQUEST:            
//            vLog(xBle, "Compare this passkey with displayed in your peer device and press 'y' or 'n': %6ld \r\n", *(uint32 *)eventParam);
            vLog(xBle, "peer provided pass: %6ld \r\n", *(uint32 *)eventParam);
            apiResult = CyBle_GapAuthPassKeyReply(cyBle_connHandle.bdHandle, 0u, CYBLE_GAP_ACCEPT_PASSKEY_REQ);
            if(apiResult != CYBLE_ERROR_OK)
                vLog(xBle, "CyBle_GapAuthPassKeyReply API Error: %d \r\n", apiResult);
            break;
                
            
        LOG_CASE(CYBLE_EVT_GATTC_WRITE_RSP)
            if (unconfirmed_writes > 0)
            {
                unconfirmed_writes--;
//                vLog(xBle, "write confirmed. unconfirmed_writes: %i\n", unconfirmed_writes);
            }
                
            //ble_events |= WriteRsp;
            break;
        
        LOG_CASE(CYBLE_EVT_GATTC_ERROR_RSP)
        {
            CYBLE_GATTC_ERR_RSP_PARAM_T *p = eventParam;
            vLog(xBle, "opcode: %i, errorcode: %i\n", p->opCode, p->errorCode);
            if (unconfirmed_reads > 0 && unconfirmed_writes == 0)
                unconfirmed_reads--;
            else if (unconfirmed_writes > 0 && unconfirmed_reads == 0)
                unconfirmed_writes--;
            else
                assert(0);
            //assert(0); // todo: decrease read or writes?
            //ble_events |= ErrorRsp;    
        }
            break;            
            
        LOG_CASE(CYBLE_EVT_GATTC_READ_RSP)
            processReadResponse(eventParam);            
            break;

            
        //LOG_CASE(CYBLE_EVT_GATTC_HANDLE_VALUE_IND)
        LOG_CASE(CYBLE_EVT_GATTC_HANDLE_VALUE_IND)
                CyBle_GattcConfirmation(((CYBLE_GATTC_HANDLE_VALUE_IND_PARAM_T *)eventParam)->connHandle);
                // fallthrough
        LOG_CASE(CYBLE_EVT_GATTC_HANDLE_VALUE_NTF)
            {
                CYBLE_GATTC_HANDLE_VALUE_NTF_PARAM_T * handleValueNotification = (CYBLE_GATTC_HANDLE_VALUE_NTF_PARAM_T *)eventParam;
                CYBLE_GATT_VALUE_T *v = &handleValueNotification->handleValPair.value;                
                
//                vLog(xBle, "data len: %i\n", v->len);
                
                switch(handleValueNotification->handleValPair.attrHandle)
                {
                    case CYBLE_MSRVXXX_DEVICE_STATUS_CHAR_HANDLE:                       
                        if (pending_stats_read > 0)
                            pending_stats_read--;
                        if (v->len == sizeof(v001stat_t))
                        {
                            memcpy(&stats, v->val, sizeof(v001stat_t));        
                            if (stats.protocolVersion != STATS_PROTOCOL_VERSION)
                            {
                                vLog(xDebug, "Error: stats protocol version does not match. theirs: %i, ours: %i\n", stats.protocolVersion, STATS_PROTOCOL_VERSION);
                                memset(&stats, 0, sizeof(stats));
                                // todo: as this device is incompatible we should probably disconnect and not discuss with it anymore
                            }
                            else
                            {     
                                //vLog(xBle, "received stats notifcation\n");
                                showStats((stats.downloadParams.crc == 0xffffffff) ? xDownload : xInfo, &stats);
                                if (dlp->endOffset - dlp->startOffset > 0 && stats.memorySize < dlp->endOffset)
                                {
                                    vLog(xDownload, "dl size was reduced from %i to %i because the other party does not have so much memory\n", 
                                        dlp->endOffset - dlp->startOffset, 
                                        stats.memorySize - dlp->startOffset);
                                    dlh.endOffset = stats.memorySize;
                                }
                            }                            
                        }
                        else
                        {
                            vLog(xDebug, "Error: wrong length for stats blob. theirs: %i, ours: %i\n", v->len, sizeof(v001stat_t));                            
                            // todo: as this device is incompatible we should probably disconnect and not discuss with it anymore
                        }
                    break;
                    case CYBLE_MSRVXXX_DOWNLOAD_CHAR_HANDLE:                        
                        processDownloadPkt(v->val, v->len);
                    break;
                        
                    default:
                        vLog(xBle, "notification received. handle: 0x%02x, %i bytes\n", handleValueNotification->handleValPair.attrHandle, v->len);
                        break;

                }
                
                
            }
            break;            
            
        LOG_CASE(CYBLE_EVT_GATTS_WRITE_REQ)
            {
                CYBLE_GATTS_WRITE_REQ_PARAM_T writeParam;
                writeParam = *(CYBLE_GATTS_WRITE_REQ_PARAM_T *)eventParam;
                
                CYBLE_GATTS_ERR_PARAM_T err;
                err.attrHandle = writeParam.handleValPair.attrHandle;
                err.errorCode = CYBLE_GATT_ERR_WRITE_NOT_PERMITTED;
                err.opcode = CYBLE_GATT_ERROR_RSP; // not sure if this is correct
                CyBle_GattsErrorRsp(cyBle_connHandle, &err ); // we don't expect any writes
                assert(0);
            }
            break;
            

#ifdef DEBUG        
        LOG_CASE( CYBLE_EVT_GAP_KEYPRESS_NOTIFICATION ); break;
        LOG_CASE( CYBLE_EVT_GAP_OOB_GENERATED_NOTIFICATION ); break;
        LOG_CASE(CYBLE_EVT_GATT_DISCONNECT_IND         ); break;
        LOG_CASE(CYBLE_EVT_GAP_ENCRYPT_CHANGE          ); break;
        LOG_CASE(CYBLE_EVT_GAP_KEYINFO_EXCHNGE_CMPLT   ); break;
        LOG_CASE(CYBLE_EVT_PENDING_FLASH_WRITE         ); break;
        LOG_CASE(CYBLE_EVT_LE_PING_AUTH_TIMEOUT        ); break;
        LOG_CASE(CYBLE_EVT_HCI_STATUS        ); break;
        LOG_CASE(CYBLE_EVT_GAP_PASSKEY_ENTRY_REQUEST        ); break;
        LOG_CASE(CYBLE_EVT_TIMEOUT        ); break;
        
//        LOG_CASE(CYBLE_EVT_GATTC_XCHNG_MTU_RSP           ); break;
        LOG_CASE(CYBLE_EVT_GATTC_READ_BY_GROUP_TYPE_RSP  ); break;
        LOG_CASE(CYBLE_EVT_GATTC_READ_BY_TYPE_RSP        ); break;
        LOG_CASE(CYBLE_EVT_GATTC_FIND_INFO_RSP           ); break;
        LOG_CASE(CYBLE_EVT_GATTC_FIND_BY_TYPE_VALUE_RSP  ); break;
//        LOG_CASE(CYBLE_EVT_GATTC_READ_RSP                ); break;
        LOG_CASE(CYBLE_EVT_GATTC_READ_MULTI_RSP          ); break;
//        LOG_CASE(CYBLE_EVT_GATTC_WRITE_RSP               ); break;
        LOG_CASE(CYBLE_EVT_GATTC_EXEC_WRITE_RSP          ); break;
//        LOG_CASE(CYBLE_EVT_GATTC_HANDLE_VALUE_NTF        ); break;
//        LOG_CASE(CYBLE_EVT_GATTC_HANDLE_VALUE_IND        ); break;
        LOG_CASE(CYBLE_EVT_GATTC_STOP_CMD_COMPLETE       ); break;
    	LOG_CASE(CYBLE_EVT_GATTC_LONG_PROCEDURE_END      ); break;
        LOG_CASE(CYBLE_EVT_GATTC_READ_BLOB_RSP) break;             
//        LOG_CASE(CYBLE_EVT_GATT_CONNECT_IND               ); break;
        LOG_CASE(CYBLE_EVT_GATTS_XCNHG_MTU_REQ         ); break;
        LOG_CASE(CYBLE_EVT_GATTS_READ_CHAR_VAL_ACCESS_REQ) break;
//        LOG_CASE(CYBLE_EVT_GATT_DISCONNECT_IND            ); break;
//        LOG_CASE(CYBLE_EVT_GATTS_XCNHG_MTU_REQ            ); break;
//        LOG_CASE(CYBLE_EVT_GATTS_WRITE_REQ                ); break;
        LOG_CASE(CYBLE_EVT_GATTS_WRITE_CMD_REQ            ); break;
        LOG_CASE(CYBLE_EVT_GATTS_PREP_WRITE_REQ           ); break;
//        LOG_CASE(CYBLE_EVT_GATTS_EXEC_WRITE_REQ           ); break;
//        LOG_CASE(CYBLE_EVT_GATTS_READ_CHAR_VAL_ACCESS_REQ ); break;
        LOG_CASE(CYBLE_EVT_GATTS_HANDLE_VALUE_CNF         ); break;
        LOG_CASE(CYBLE_EVT_GATTS_DATA_SIGNED_CMD_REQ      ); break;        
#endif        
            
        default:
            vLog(xBle, "unknown case: %i (0x%x)\n", eventCode, eventCode);
            break;
    }
}


void  HandleBleProcessing(void)
{
    if (!bleConnected)
        return;    
   

    if (ble_fail >= MAX_BLE_FAILURES)
    {
        for (int i = 0; i < 5; i++)
            vLog(xBUG, " BLE stack failed! \n");
        
        ble_fail = 0;
        CyBle_Stop();
        resetAllVars();
        apiResult = CyBle_Start(Ble_Stack_Handler);
        if(apiResult != CYBLE_ERROR_OK)
            vLog(xBle, "CyBle_Start Error: %d\n", apiResult);
        assert(CYBLE_ERROR_OK == apiResult);
    }
 }

/* [] END OF FILE */
