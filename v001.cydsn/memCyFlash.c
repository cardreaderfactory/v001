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
#include "common.h"
#include "debug.h"
#include "xprintf.h"
#include "device.h"
//#include "bluetooth.h"
#include "memCyFlash.h"
#include "memory.h"


const uint32_t startMemoryBuf = 0;
static void * startMemory = NULL;
static volatile void * writePointer = NULL;
static const void * flashEnd = (uint32_t *)(CY_FLASH_BASE + CY_FLASH_SIZE);


void *flashSp2()
{
    return (void*)flashRead_uint32(&startMemoryBuf);
}

void *flashSp()
{
    return (void*)startMemory;
}

bool flashSetWp(void *p)
{
    assert(flashSp() == flashSp2());
    if (p >= startMemory &&
        p >= (void*)MIN_FIRMWARE_SIZE &&
        p <= flashEnd)
    {
        writePointer = p;
        stats.memoryUsed = writePointer - startMemory;
//        vLog(xInfo, "Flash writePointer was changed to %lu\n", writePointer);
        return true;
    }
    else
    {
        vLog(xBUG, "requested invalid change of write pointer: %lu, startMemory: %lu", p, startMemory);
        assert(0);
        return false;
    }
}

void *flashWp()
{
    return (void*)writePointer;
}

void *flashEp()
{
    return (void*)flashEnd;
}

void *rowAddr(uint32_t row)
{
    return (void *)((CY_FLASH_BASE + row * CY_FLASH_SIZEOF_ROW));
}

#if 1  /* fast. beware: it happened to return results within the firmware with search area too low */

    #define SEARCH_LEN 1024 /* looking for 1024 * 4 = 4k block */
    
bool isMemoryEmpty(void *p)
{

    if ( p >= flashEnd )
    {
        //vLog(xInfo, "found data at %lu (addr >= df_totalMemory)\n", addr);
        return false;
    }
    // addressing misaligned uint32_t crashes on this chip/compiler
	// patch:
    uint8_t *c = p;
    while ((uint32_t)c % 4 != 0)
    {
        if (*c != 0)
        {
//            vLog(xInfo, "found data at %lu, i = %i\n", p, (void*)c-p);
            return false;
        }
        c++;
    }
	// end patch
    
    uint32_t * addr = (void *)c;
//    vLog(xInfo, "addr = %i, c = %i\n", addr, c);

    for(int i = 0; i < SEARCH_LEN; i++)
    {
        if ((void*)&addr[i] >= flashEnd)
            return true;
        if (addr[i] != 0)
        {
//            vLog(xInfo, "found data at %lu, i = %i\n", addr, i);
            return false;
        }
    }

    return true;
}

void * findUsedMemory(void)
{
    void * start;
    void * stop;
    void * addr;
    void * oldaddr;

    uint32_t ms = GetTick();

    start = (void*)(CY_FLASH_BASE+MIN_FIRMWARE_SIZE);
    if (start < flashSp())
        start = flashSp();
    stop  = (void*)flashEnd;
    addr  = 0;

    /* binary search */
    while (start != stop)
    {
        oldaddr = addr;
        addr = start + ((stop - start) / 2);
        if (addr == oldaddr)
        {
//            xprintf("old addr = %lu, addr = %lu, start = %lu, stop = %lu\n", oldaddr, addr, start, stop);
            break;
        }

//        xprintf("start = %lu, stop = %lu, addr = %lu\n", start, stop, addr);

        if (isMemoryEmpty(addr))
            stop = addr;
        else
            start = addr;
    }
    
    vLog(xDebug, "findUsedMemory took %lums and found addr: %lu\n", GetTick() - ms, stop);

    if (stop > flashEnd)
    {
        assert(0);
        return NULL;
    }
    
    if ((uint32_t)stop % AES_BLOCKSIZE != 0)
        return (void *)(((uint32_t)stop / AES_BLOCKSIZE + 1) * AES_BLOCKSIZE);
    else 
        return stop;    
}
#else

void * findUsedMemory(void)
{
    uint32_t ms = GetTick();
    uint32_t * addr = (uint32_t *)flashEnd;
    uint32_t d = 0;
    
    for (addr = ((uint32_t*)flashEnd) - 1; addr > (uint32_t *)MIN_FIRMWARE_SIZE; addr--)
    {
//        if (d < 3)
//        {
//            vLog(xDebug, "checking %lu\n", addr);
//            d++;
//        }
        if (*addr != 0)
        {
            addr++;
            break;
        }
    }
    vLog(xDebug, "findUsedMemory took %lums and found addr: %lu\n", GetTick() - ms, addr);
    return addr;
}
#endif


bool flashWriteRow(int row, void *buf)
{
    //todo correct time stamp, safeguard against shutdown
#if LOG & LOG_FLASH_WRITE_ROW
   vLog(xBle, "writing row %i\n", row);                            
   flash_printPageBuf(buf);
#endif

#if LOG & LOG_FLASH_DATA_OVERWRITE
    uint8_t *b = buf;
    bool isErasing = true;
    for (uint16_t i = 0; i < CY_FLASH_SIZEOF_ROW; i++)
        if (b[i] != 0)
        {
            isErasing = false;
            break;
        }
        
    if (!isErasing)
    {
        uint8_t *p = rowAddr(row);
        for (uint16_t i = 0; i < CY_FLASH_SIZEOF_ROW; i++)
            if (b[i] != p[i] && p[i] != 0)
            {
                vLog(xDebug, "Warning: overwriting useful data in row %i at offset: %i\n", row, i);
                flash_printPage(row);
                break;
            }
    }
#endif    
    wdtReset();// reset the watchdog to ensure it won't reset us during programming    
    while (UART_SpiUartGetTxBufferSize() != 0 && UART_GET_TX_FIFO_SR_VALID != 0);
    if (CY_SYS_FLASH_SUCCESS != CySysFlashWriteRow(row, buf))
    {
        vLog(xErr, "cannot write row %i\n", row);
        assert(false);
        return false;
    }
    return true;
}

uint16_t flashFreeBytesInPage(void *p)
{
    int32_t r = ((uint32_t)p - CY_FLASH_BASE) % CY_FLASH_SIZEOF_ROW;
//    vLog(xDebug, "pointer in page %lu at ofsset: %i, p: %lu\n", ((uint32_t)p - CY_FLASH_BASE) / CY_FLASH_SIZEOF_ROW, r, p);
    assert(r>=0);
    assert(p <= flashEnd);
    if (r < 0)
        return 0;
    return CY_FLASH_SIZEOF_ROW - r;
}

bool flashWrite(const void *p, void *buf, uint16_t len)
{
    uint32_t    addr                            = (uint32_t)p;
    uint32_t    rowid                           = (addr - CY_FLASH_BASE) / CY_FLASH_SIZEOF_ROW;
    uint32_t    offset                          = (addr - CY_FLASH_BASE) % CY_FLASH_SIZEOF_ROW;
    uint16_t    l;
    uint16_t    w = 0;
    uint8_t     rowBuf[CY_FLASH_SIZEOF_ROW];

    if (len == 0)
        return true;
    
    if (p >= flashEp())
        return false;
    
    assert(flashSp() == flashSp2());
    
//    vLog(xBle, "%s: writing at %lu (0x%08x) - row %lu (%lu), len: %u\n", __FUNCTION__, p - flashSp(), p, rowid, rowid-(uint32_t)flashSp()/CY_FLASH_SIZEOF_ROW, len);

    if ( (uint32_t)p < MIN_FIRMWARE_SIZE || p < flashSp())
    {
        vLog(xBUG, "attempted to write in a write protected area at %lu\n", p);
        return false;
    }
    assert(p >= flashSp());
    
//    vLog(xBle, "this is what is in the buffer:\n");                            
//    flash_printBuf(buf, len);                    

    l = 0;
    w = offset;
    while (len)
    {
        
        if (rowid >= CY_FLASH_NUMBER_ROWS)
        {
            vLog(xErr, "attempted to write out of memory. %i bytes not written.\n", len);
            return false;
        }
		
#if LOG & LOG_FLASH_DATA_OVERWRITE
        bool overwrite;
		overwrite = false;
#endif    

        l = min(CY_FLASH_SIZEOF_ROW, len);
//        vLog(xBle, "writing at row: %i offset: %i, %i bytes. remaining to write: %i\n", rowid, offset, l, len);
        if (l < CY_FLASH_SIZEOF_ROW)
        {
            vLog(xInfo, "not overwriting the whole row. reading row %i\n", rowid);            
            memcpy(rowBuf, rowAddr(rowid), CY_FLASH_SIZEOF_ROW); // read current page into memory (so we don't erase useful data)
            
#if LOG & LOG_FLASH_DATA_OVERWRITE
            for(int i = 0, j=offset; i < CY_FLASH_SIZEOF_ROW && j < l; i++, j++)
                if (rowBuf[j] != 0 && ((uint8_t*)buf)[i] != 0)
                {
                    vLog(xInfo, "Warning: overwriting non empty data at offset=%i, rowbuf[j]=%02x, buf=%02x!\n", j, rowBuf[j], ((uint8_t*)buf)[i]);
                    flash_printBuf(rowBuf, CY_FLASH_SIZEOF_ROW);
                    vLog(xInfo, "overwriting with this buffer\n");
                    flash_printBuf(buf,l);
                    myputchar('\n');
                    overwrite=true;
                    break;
                }
#endif                

//            vLog(xBle, "this is what we read:\n");            
//            flash_printPageBuf(rowBuf);
        }

#if LOG & LOG_CY_FLASH_WRITE
        vLog(xBle, "this is what is in flash before writing:\n");                            
        flash_printPage(rowid);                
        vLog(xBle, "this is what is in the buffer:\n");                            
        flash_printBuf(buf, l);                
#endif            
        
        memcpy((uint8_t *)rowBuf+offset, buf, l);
#if LOG & LOG_CY_FLASH_WRITE        
        vLog(xBle, "this is what we are writing: (%i bytes changed at offset: %i)\n", l, offset);
        flash_printPageBuf(rowBuf);        
#endif        
        
        w += l;
        offset = 0;
        len -= l;
        wdtReset();
        flashWriteRow(rowid, rowBuf);
        
#if LOG & LOG_FLASH_DATA_OVERWRITE
        if(overwrite)
        {
            vLog(xInfo, "overwritten page:\n");
            flash_printPage(rowid);            
        }
#endif        
        

#if LOG & LOG_CY_FLASH_WRITE        
        vLog(xBle, "this is what is in flash after writing:\n");                            
        flash_printPage(rowid);
#endif        

        if (len != 0)
        {
            rowid++;
            w = 0;
        }
    }
    
    assert(w <= CY_FLASH_SIZEOF_ROW);    
    flashSetWp(rowAddr(rowid) + w);        

    return true;
}


void flashWrite_uint32(const void *p, uint32_t value)
{
    uint32_t addr = (uint32_t)p;
    assert(p < flashEnd);
    
    //uint32_t addr = (uint32_t)p;
    char rowBuf[CY_FLASH_SIZEOF_ROW];
    uint32_t rowid = (addr - CY_FLASH_BASE) / CY_FLASH_SIZEOF_ROW;
    uint32_t offset = (addr - CY_FLASH_BASE) % CY_FLASH_SIZEOF_ROW;
    assert(offset+4 < CY_FLASH_SIZEOF_ROW);


//    vLog(xInfo, "offset: %lu, offset/4: %lu, offset%4: %lu,rowBuf[offset/4]: %lu\n", offset, offset/4, offset%4, rowBuf[offset/4]);
    
    memcpy(rowBuf, rowAddr(rowid), CY_FLASH_SIZEOF_ROW);
    memcpy(rowBuf+offset, &value, 4);

    flashWriteRow(rowid, rowBuf);
}

#pragma GCC push_options
#pragma GCC optimize ("O0")    
uint32_t flashRead_uint32(const void *p)
{
    uint32_t d;
    memcpy(&d, p, 4);    
    return d;
    //return *(uint32_t *)p;
}
#pragma GCC pop_options

void flashErase(void)
{
    uint32_t *p = startMemory;
    if (p == NULL)
    {
        vLog(xBUG, "memory uninitialized\n");
        return;
    }
    uint32_t rowBuf[CY_FLASH_SIZEOF_ROW/sizeof(uint32_t)];
    uint32_t rowid = ((uint32_t)p - CY_FLASH_BASE) / CY_FLASH_SIZEOF_ROW;

    vLog(xInfo, "memory starting from: %lu (0x%08lx), available: %lu, row: %lu\n", p, p, CY_FLASH_BASE+CY_FLASH_SIZE-(uint32_t)p, rowid);

    assert(((uint32_t)p - CY_FLASH_BASE) % CY_FLASH_SIZEOF_ROW == 0);
    assert(flashSp() == flashSp2());

    memset(rowBuf, 0, CY_FLASH_SIZEOF_ROW);
    
    uint32_t counter = GetTick();
    uint32 i;
    uint32 cnt = 0;
    bool needLf = false;
    vLog(xDebug, "writing from %i to %i\n", rowid, CY_FLASH_NUMBER_ROWS);
    for (i = rowid; i < CY_FLASH_NUMBER_ROWS; i++)
    {

        if (memcmp(rowBuf, rowAddr(i), CY_FLASH_SIZEOF_ROW)==0)
        {
            //xprintf("%4lu=  ", i);
        }
        else
        {
            if (!flashWriteRow(i, rowBuf))
                break;
            needLf = true;
            xprintf("%4lu   ", i);
            cnt++;
            if (cnt%16 == 0)
            {
                myputchar('\n');
                needLf = false;
            }
        }

    }
    if (needLf)
        myputchar('\n');

    counter = GetTick()-counter;
    i-=rowid;
    vLog(xDebug, "%lu flash pages erased in %lu ms (%ibytes/sec, %i ms/page) \n", cnt, counter, (cnt*CY_FLASH_SIZEOF_ROW * 1000UL) / counter, counter/cnt);

    flashSetWp(findUsedMemory());
}


#if MODULES & MOD_CY_FLASH_FILL
void flashFill(void)
{
//    flash_printPage(0);
//    flash_printPage(1);

    //vLog(xInfo, "start search\n");
    uint32_t *p = startMemory;
    if (p == NULL)
    {
        vLog(xBUG, "memory uninitialized\n");
        return;
    }

    uint32_t rowBuf[CY_FLASH_SIZEOF_ROW/sizeof(uint32_t)];
    uint32_t rowid = ((uint32_t)p - CY_FLASH_BASE) / CY_FLASH_SIZEOF_ROW;

    vLog(xInfo, "memory starting from: %lu (0x%08lx), available: %lu, row: %lu\n", p, p, CY_FLASH_BASE+CY_FLASH_SIZE-(uint32_t)p, rowid);

    assert(((uint32_t)p - CY_FLASH_BASE) % CY_FLASH_SIZEOF_ROW == 0);
    assert(flashSp() == flashSp2());

    for(uint32_t *x = startMemory; (void*)x < flashEnd; x++)
        if (*x != 0)
        {
            vLog(xInfo, "Warning: writing garbage over non zero data (found %lu @ addr: %lu)\n", *x, x);
            flash_printPage((uint32_t)x/CY_FLASH_SIZEOF_ROW-CY_FLASH_BASE);
            break;
        }

    for(uint16 i = 0; i < CY_FLASH_SIZEOF_ROW; i+=4)
        rowBuf[i/4] = i+((i+1)<<8)+((i+2)<<16)+((i+3)<<24);


    uint32_t counter = GetTick();
    uint32 i;
    vLog(xDebug, "writing from %i to %i", rowid, CY_FLASH_NUMBER_ROWS);
    for (i = rowid; i < CY_FLASH_NUMBER_ROWS; i++)
    {
        char *c;
        if ((i-rowid)%16 == 0)
            myputchar('\n');

        if (memcmp(rowBuf, rowAddr(i), CY_FLASH_SIZEOF_ROW)==0)
            c="*";
        else
            c=" ";

        xprintf("%4lu%s  ", i, c);

        wdtReset();// reset the watchdog to ensure it won't reset us during programming

        if (!flashWriteRow(i, rowBuf))
            break;
    }
    myputchar('\n');

    counter = GetTick()-counter;
    i-=rowid;
    vLog(xDebug, "%lu flash pages were written in %lu ms (%ibytes/sec, %i ms/page) \n", i, counter, (i*CY_FLASH_SIZEOF_ROW * 1000UL) / counter, counter/i);

#if MODULES & MOD_CY_FLASH_FILL_VERIFY
    vLog(xDebug, "verifying from %i to %i", rowid, CY_FLASH_NUMBER_ROWS);
    for(uint32_t j = rowid; j < CY_FLASH_NUMBER_ROWS; j++)
    {
        if ((j-rowid)%16 == 0)
            myputchar('\n');
        xprintf("%4lu  ", j);

        memset(rowBuf, 0 , CY_FLASH_SIZEOF_ROW);
        memcpy(rowBuf, rowAddr(j), CY_FLASH_SIZEOF_ROW);
        for(uint32_t i = 0; i < CY_FLASH_SIZEOF_ROW/4; i++)
        {
            uint32_t expected = 0;
            uint32_t k = i*4;
            expected = k+((k+1)<<8)+((k+2)<<16)+((k+3)<<24);
            if (rowBuf[i] != expected)
            {
                vLog(xInfo, "\n");
                vLog(xInfo, "memory verification failed at page %i, addr 0x%08x (%i) value: 0x%08x, expected: 0x%08x)\n", i, i, rowBuf[i], expected);
                break;
            }
        }
    }
    myputchar('\n');
#endif    

    flashSetWp(findUsedMemory());
}
#endif


void CyFlash_Start(void)
{
    startMemory = (uint32_t *)flashRead_uint32(&startMemoryBuf);

//    xprintf("start memory value: %lu\n", startMemory);

    if (startMemory == NULL)
    {
        startMemory = findUsedMemory();
        if (startMemory != NULL)
        {
            uint32_t rowid = ((uint32_t)startMemory - CY_FLASH_BASE) / CY_FLASH_SIZEOF_ROW;
            if (((uint32_t)startMemory - CY_FLASH_BASE) % CY_FLASH_SIZEOF_ROW != 0)
                rowid++;

            startMemory = (void *)(CY_FLASH_BASE + (rowid+1)*CY_FLASH_SIZEOF_ROW);
            flashWrite_uint32(&startMemoryBuf, (uint32_t)startMemory);
            startMemory = (uint32_t *)flashRead_uint32(&startMemoryBuf);

            vLog(xInfo, "free mem at: %lu (0x%08lx), available: %lu\n", startMemory, startMemory, CY_FLASH_BASE+CY_FLASH_SIZE-(uint32_t)startMemory);
        }
        else
        {
            vLog(xInfo, "memory full\n");
            startMemory = (void*)flashEnd;
        }
    }
    
    if (startMemory < (void*)MIN_FIRMWARE_SIZE)
    {
        vLog(xBUG, "FATAL: start memory: %lu is lower than MIN_FIRMWARE_SIZE. cannot continue\n", startMemory);
        assert(0);
        while(1);
    }
//    xprintf("start memory value: %lu\n", startMemory);
   stats.memorySize = flashEnd - startMemory;
   flashSetWp(findUsedMemory());
   vLog(xInfo, "Flash writePointer was changed to %lu\n", writePointer);
    
    
    stats.memoryUsed = writePointer - startMemory;

//    vLog(xInfo, "flashStart: %lu\n", flashSp());
//    vLog(xInfo, "flashWrite: %lu\n", flashWp());
//    vLog(xInfo, "flashEnd  : %lu\n", flashEp());


}
