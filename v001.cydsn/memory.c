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
#include "CyFlash.h"
#include "common.h"
#include "debug.h"
#include "xprintf.h"
#include "memCyFlash.h"
#include "device.h"
#include "aes.h"



#define PAGE_SIZE CY_FLASH_SIZEOF_ROW
#define CACHE_SIZE (4*PAGE_SIZE)


const uint8_t   keyStorage[]            = AES_KEY;
const uint8_t  *key                     = keyStorage;
uint16_t        cacheCursor             = 0;
const uint8_t   initialChain[]          = {0x7F, 0x83, 0x71, 0x0D, 0x4F, 0x1F, 0x21, 0x71, 0x2a, 0xDC, 0x4A, 0x7f, 0x74, 0x33, 0x75, 0xC8};
uint8_t         cache[CACHE_SIZE];


#define SPACE 16
#define NEW_LINE (SPACE*2)

void flash_printAscii(uint8 *p, int len)
{
    int l = min(NEW_LINE, len);
    for(int j = 0; j < l; j++)
    {
        if (p[j] < 32 || p[j] >= 127)
            myputchar('_');
        else
            myputchar(p[j]);
            
//        if ((j+1) % SPACE == 0)
//            xprintf(" ");
    }
    
    for(int j = l; j < NEW_LINE; j++)
        myputchar(' ');
        
    myputchar('|');
            
//    if (l != 0)
//        xprintf("\n");
}

void printEolAscii(uint8_t *p, uint8_t *d, int offset, int len)
{
    myputchar('|');
    flash_printAscii(p + offset, len);
    if (d != NULL) 
    {
//        myputchar(' ');
        flash_printAscii(d+offset, len);
    }
    myputchar('\n');
}

void flash_printBufDecrypt(void *v, int len, uint8 *d)
{
    uint8 *p = v;    
    uint16_t n = NEW_LINE;
    int nl = 0;
        
    for(uint16_t i = 0, s = SPACE; i < len; i++, s--, n--)
    {
        if (s == 0)
        {
            myputchar(' ');
            s = SPACE;
        }
        
        if (n == 0)
        {
            printEolAscii(p, d, i - NEW_LINE, NEW_LINE);
            n = NEW_LINE;
            nl = i;
        }
                       
        if (n == NEW_LINE)
        {
            uint32_t addr = (uint32_t)&p[i];
            
            if (addr & 0x20000000)
            {
                xprintf("RAM   ");
                addr &= ~0x20000000;
            }
            else
            {
                xprintf("FLASH ");
            }
            
            xprintf("%06lu (0x%05x): ", addr, addr);
        }
            
        xprintf("%02x", p[i]);
        //nl = false;
    }
    
    if (len != 0)
    {   
        myputchar(' ');
        assert(len>nl);
        printEolAscii(p, d, nl, len-nl);
    }
//    xprintf("len = %i, nl = %i\n", len, nl);
}

void flash_printBuf(void *v, int len)
{
    flash_printBufDecrypt(v, len, NULL);
}


void flash_printPageBufDecrypt(void *v, uint8 *d)
{
    //flash_printBuf(v, CY_FLASH_SIZEOF_ROW);
    flash_printBufDecrypt(v, CY_FLASH_SIZEOF_ROW, d);
}

void flash_printPageBuf(void *v)
{
    flash_printPageBufDecrypt(v, NULL);
}

void flash_printPage(uint16_t row_id)
{
    uint8_t *p = NULL;
    if (row_id >= CY_FLASH_NUMBER_ROWS || row_id <= MIN_FIRMWARE_SIZE / CY_FLASH_SIZEOF_ROW)
    {
        vLog(xBUG, "Display of row id %i is forbidden\n", row_id);
        return;
    }
    assert(row_id < CY_FLASH_NUMBER_ROWS);
    assert(flashSp() == flashSp2());
    
    wdtReset();

#if MODULES & MOD_AES_DECRYPT
    static int last_row = 0;
    uint8_t chainCipherBlock[16];
    uint8_t d[CY_FLASH_SIZEOF_ROW];
    if (last_row != row_id-1)
    {
#if LOG & LOG_AES_DECRYPT        
        vLog(xDebug, "page %i - rebuilding decrypt block chain\n", row_id);
#endif        
        aesInit(key);
        memcpy(chainCipherBlock, initialChain, 16);        
//        vLog(xDebug, "chainCipherBlock start:\n");
//        flash_printBuf(chainCipherBlock, 16);        
        for(p = flashSp(); p < (uint8_t*)rowAddr(row_id); p+=16)
        {
            memcpy(d, p, 16);
            aesDecrypt(d, chainCipherBlock);        
//            vLog(xDebug, "addr %i, decrypted buf:\n", p);
//            flash_printBuf(d, 16);
        }
        assert(p == rowAddr(row_id));
#if LOG & LOG_AES_DECRYPT
        vLog(xDebug, "chainCipherBlock end:\n");
        flash_printBuf(chainCipherBlock, 16);
//        vLog(xDebug, "decripted block:\n");
//        flash_printBuf(d, 16);
#endif    
    }
    else
    {
#if LOG & LOG_AES_DECRYPT
        vLog(xDebug, "page %i - using previous block chain\n", row_id);
        flash_printBuf(chainCipherBlock, 16);
#endif        
        p = rowAddr(row_id);
    }
    
    memcpy(d, p, CY_FLASH_SIZEOF_ROW);
    for (uint16 i = 0; i < CY_FLASH_SIZEOF_ROW; i+=16)
    {
#if LOG & LOG_AES_DECRYPT        
        vLog(xDebug, "    Decription : ");
        for (int j = i; j < i+AES_BLOCKSIZE; j++)
            xprintf("%02x", d[j]);
        xprintf(" --> ");
#endif        
        aesDecrypt((uint8_t *)d + i, chainCipherBlock);
#if LOG & LOG_AES_DECRYPT        
        for (int j = i; j < i+AES_BLOCKSIZE; j++)
            xprintf("%02x", d[j]);
        myputchar(' ');
        for (int j = i; j < i+AES_BLOCKSIZE; j++)
        {
            if (d[j] < 32 || d[j] >= 127)
                myputchar('_');
            else
                myputchar(d[j]);
        }
            
        myputchar('\n');
#endif        
    }
        
    last_row = row_id;
    myputchar('\n');        
#else
    uint8_t *d = NULL;
    p = rowAddr(row_id);
#endif    
    xprintf("page %i (memory %i to %i) p: %lx\n", row_id, rowAddr(row_id), rowAddr(row_id+1)-1, p);
    flash_printPageBufDecrypt(p,d);
    
}

bool writeCache(uint16 len)
{   
#if LOG & LOG_FLASH_CACHE_INFO
    vLog(xDebug, "%s: writing at %i (0x%08lx) from cache. len: %i\n", __FUNCTION__, flashWp()-flashSp(), flashWp(), len);
#endif    
    assert(flashSp() == flashSp2());
    if (len > CACHE_SIZE)
    {
        assert(0);
        len = CACHE_SIZE;
    }
    const uint8_t * prevAesBlock;
    uint8 *p = cache;
    // put the block chain into aes buffer
    if (flashSp() >= flashWp())
    {
//        vLog(xDebug, "    using initial chain\n");
        prevAesBlock = initialChain;
    }
    else
    {
        prevAesBlock = flashWp() - AES_BLOCKSIZE;
    }

//    vLog(xDebug, "    prevAESblock at %i (0x%08lx), diff:%i\n", prevAesBlock, prevAesBlock, (int)flashWp() - (int)prevAesBlock);

#if (CACHE_SIZE % AES_BLOCKSIZE != 0) || (CACHE_SIZE < AES_BLOCKSIZE)
    #error CACHE_SIZE must be bigger and a multiple of AES_BLOCKSIZE
#endif

#if (CACHE_SIZE % CY_FLASH_SIZEOF_ROW != 0) || (CACHE_SIZE <= CY_FLASH_SIZEOF_ROW)
    #error CACHE_SIZE must be bigger and a multiple of CY_FLASH_SIZEOF_ROW
#endif

    if (key == NULL)
    {            
        vLog(xErr, "warning: not using encryption!\n");
        assert(0);
        return false;
    }
    
    
#if LOG & LOG_FLASH_CACHE
    if (prevAesBlock == initialChain)        
    {
        vLog(xDebug, "    key: ");
        for (int i = 0; i < AES_BLOCKSIZE; i++)
            xprintf("%02x", key[i]);
        myputchar('\n');
        vLog(xDebug, "    key reversed: ");
        for (int i = AES_BLOCKSIZE; i > 0; i--)
            xprintf("%02x", key[i-1]);
        myputchar('\n');
    }
#endif    

    p = cache; // pointer where we read from
    uint16_t l = 0;  // bytes from the buffer remaining to write
    uint16_t r = flashFreeBytesInPage(flashWp()); // bytes left in the current row
    uint16_t w = 0;
    while (len > 0)
    {
        l = min(len, AES_BLOCKSIZE);
        assert(r % AES_BLOCKSIZE == 0);
        assert(r>=l);
//        vLog(xDebug, "    l=%i, r=%i, len=%i, w=%i, offset=%i\n", l, r, len, w, p-cache);

        if (l < AES_BLOCKSIZE)
        {
//            vLog(xDebug, "    cache[%lu..%lu) = 0\n", p+l-cache, p+AES_BLOCKSIZE-cache);
            memset(p + l, 0, AES_BLOCKSIZE - l);
        }

#if LOG & LOG_FLASH_CACHE
        if (prevAesBlock == initialChain)
        {
            vLog(xDebug, "    Before encryption: ");
            for (int i = 0; i < AES_BLOCKSIZE; i++)
                xprintf("%02x", p[i]);
            myputchar(' ');
            for (int i = 0; i < AES_BLOCKSIZE; i++)
                xprintf("%c", p[i]);
            myputchar('\n');
        }
#endif
        for (int i = 0; i < AES_BLOCKSIZE; i++)
        {
            p[i] ^= prevAesBlock[i]; /* xor bytes (buffer with chainBlock) */
        }
        
#if LOG & LOG_FLASH_CACHE
        if (prevAesBlock == initialChain)
        {
            vLog(xDebug, "    CBC block: ");
            for (int i = 0; i < AES_BLOCKSIZE; i++)
                xprintf("%02x", p[i]);
            myputchar('\n');
            vLog(xDebug, "    CBC block reversed: ");
            for (int i = AES_BLOCKSIZE; i > 0; i--)
                xprintf("%02x", p[i-1]);
            myputchar('\n');
        }
#endif

        CyBle_AesEncrypt(p, (uint8 *)key, p);
        
#if LOG & LOG_FLASH_CACHE
        vLog(xDebug, "    After encryption: ");
        for (int i = 0; i < AES_BLOCKSIZE; i++)
            xprintf("%02x", p[i]);
        myputchar('\n');
        
        if (prevAesBlock == initialChain)
        {
            vLog(xDebug, "    After encryption reversed: ");
            for (int i = AES_BLOCKSIZE; i > 0; i--)
                xprintf("%02x", p[i-1]);
            myputchar('\n');
        }
#endif

        assert(r>=l);
        r -= l;
        w += AES_BLOCKSIZE;
        prevAesBlock = p;
        p += AES_BLOCKSIZE;

        if (r==0)
        {
#if LOG & LOG_FLASH_CACHE
            vLog(xDebug, "    writing from %lu (0x%lx), len: %i, offset:%i\n", flashWp()-flashSp(), flashWp(), w, p-w-cache);
            flash_printPageBuf(p-w);
#endif    
            
            if (flashWrite(flashWp(), p-w, w) == false)
                return false;
            w = 0;
#if LOG & LOG_FLASH_CACHE
            vLog(xDebug, "    after flash write, r = %i, flashFreeBytesInPage=%i\n", r, flashFreeBytesInPage(flashWp()));            
#endif            
            r = flashFreeBytesInPage(flashWp());
        }

        assert(len>=l);
        len -= l;
        if (len == 0)
            break;

    };

//    vLog(xDebug, "    l=%i, r=%i, len=%i, w=%i, offset=%i\n", l, r, len, w, p-cache);
    if (w != 0)
    {
#if LOG & LOG_FLASH_CACHE
        vLog(xDebug, "    writing from %lu (0x%lx), len: %i, offset:%i\n", flashWp()-flashSp(), flashWp(), w, p-w-cache);
#endif    
        assert(w % AES_BLOCKSIZE == 0);
        if (flashWrite(flashWp(), p-w, w) == false)
            return false;
    }
    
#if LOG & LOG_FLASH_CACHE
    for(uint32_t i = (uint32_t)flashSp()/CY_FLASH_SIZEOF_ROW; i < 1+(uint32_t)flashWp()/CY_FLASH_SIZEOF_ROW; i++)
        flash_printPage(i);
#endif        
    
    return true;
}

bool write(const void *buf, uint16_t length)
{
    assert(flashSp() != NULL);
    assert(flashEp() != NULL);
    assert(flashWp() != NULL);
    
    assert(flashEp() >= flashWp());
    if (flashEp()  <= flashWp())
    {
        vLog(xInfo, "out of memory\n");
        return false;
    }
    assert(flashWp() >= flashSp());

    uint16_t l; // bytes from the current buffer remaining to be written

    if (length == 0)
        return true;
#if LOG & LOG_FLASH_WRITE    
    vLog(xDebug, "%s: len: %i: ", __FUNCTION__, length);
    myputchar('"');
    for(int i = 0; i< length; i++)
        myputchar(((char*)buf)[i]);
    myputchar('"');
    myputchar('\n');
#endif    
    while (length)
    {
        l = min(length, CACHE_SIZE - cacheCursor);
//        vLog(xDebug, "    l=%i, cacheCursor:%i, length: %i\n", l, cacheCursor, length);

        memcpy((uint8_t*)cache + cacheCursor, buf, l);
        buf+=l;
        cacheCursor += l;
        assert(cacheCursor <= CACHE_SIZE);
        assert(length >= l);
        length -= l;
        

        if (cacheCursor == CACHE_SIZE)
        {
            writeCache(cacheCursor);
            cacheCursor = 0;
        }
    }
//    vLog(xDebug, "done. length: %i\n", length);

    return (length == 0);
}

void flush(void)
{
    if (cacheCursor != 0)
    {
        writeCache(cacheCursor);
        cacheCursor = 0;
    }
}



void setAesKey(const uint8_t *k)
{
    key = k;
    #warning todo: copy this key into flash....
}

#if MODULES & MOD_AES_DECRYPT
void aesTest(void)
{
    uint8_t data[] = "1234567890abcdef";
    uint8_t keys[16];
    uint8_t chainBlock[16];
    uint8_t enc[16];
    uint8_t *d = data;
    uint8_t *c = chainBlock;
    uint8_t *k = keys;
    
    memset(chainBlock, 0, sizeof(chainBlock));
    memcpy(k, key, 16);
//    memcpy(c, d, 16);
    aesInit(k);
    vLog(xDebug, "key       : ");     flip(k);flash_printBuf((void*)k, 16);flip(k);
    vLog(xDebug, "data      : ");     flip(d);flash_printBuf(d, 16);flip(d);
    vLog(xDebug, "chain     : ");     flip(c);flash_printBuf(c, 16);flip(c);
    aesDecrypt(d, c);
    vLog(xDebug, "decrypt   : ");     flip(d); flash_printBuf(d, 16);    flip(d);
    CyBle_AesEncrypt(d, (uint8 *)k, enc);    
    vLog(xDebug, "re-encrypt: ");      flip(enc); flash_printBuf(enc, 16);flip(enc);
    
}
#endif


