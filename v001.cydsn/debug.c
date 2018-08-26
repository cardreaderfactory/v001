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
* File Name: debug.c
*
* Version 1.0
*
* Description:
*  Common BLE application code for printout debug messages.
*
*/

#include <stdarg.h>
#include "project.h"
#include "stdio.h"
#include "common.h"
#include "debug.h"
#include "xprintf.h"
#include "device.h"

bool        stop_display    = false;
uint32_t    vLogFlags       = ALL_LOGS_ON;
void        (*xfunc_out)(unsigned char);
static int  mll       = 0;


int maxLogLen(void)
{
    if (mll == 0)
    {
        for (int i = 0; i < xCount; i++)
            mll = max(strlen(log2st(i)), mll);

        mll += 1; // skipping first letter and adding a space
    }
    
    return mll;
}

void myputchar(unsigned char c)
{
    if (stop_display)
        return;

    debug_stuff(c);
    UART_UartPutChar(c);
}

char *log2st(int t)
{
    switch (t)
    {
        E2ST(xInfo        )
        E2ST(xBUG         )    
        E2ST(xErr         )
        E2ST(xBle         )    
        E2ST(xDownload    )
        E2ST(xPeakVrbs    )
        E2ST(xPeakValue   )
        E2ST(xPeakBits    )
        E2ST(xProfiler    )
        E2ST(xDebug       )    
    }        
    return "???";
}

const char *parse_filename(const char *st)
{
    const char *p = st;
    while (*p != 0) p++;
//    while (p > st && *(p-1) != '/' && *(p-1) != '\\' && *(p-1) != '_') p--;
    while (p > st && *(p-1) != '/' && *(p-1) != '\\') p--;
    return p;
}

#ifdef TRACE
static void print_level(uint8_t level)
{
    for (uint8_t i = 0; i < level; i++)
        xprintf("|    ");
}
#endif

#ifdef SHOW_TIMESTAMPS
static bool _timestamps = true;

int print_time(void)
{
    int i = 0;
    char buf[16];
    uint32_t m = GetTick() * 1000 / 1024;
//    sprintf(buf, "%3lu %5lu.%03lu ",  seconds, miliseconds/1000, miliseconds%1000);
    sprintf(buf, "%02lu:%02lu.%03lu ",  timestamp/60, timestamp % 60, m%1000);

    char *p = buf;
    while (*p != 0)
    {
        myputchar(*p);
        p++;
        i++;
    }

    return i;
}

bool show_timestamps(bool val)
{
    bool before = _timestamps;
    _timestamps = val;
    return before;
}
#endif

void debug_stuff(char ch)
{
#ifdef CLEAR_EOL
    static volatile int pad = 0;
    if (ch == '\n' || ch == '\r')
    {
        while (pad>0)
        {
            UART_UartPutChar(' ');
            pad--;
        }
    }
    else if (ch == '\b')
    {
        pad++;
    }
    else if (pad>0)
    {
        pad--;
    }
#endif

#ifdef SHOW_TIMESTAMPS
    static bool timestamp_due = true;
    if (_timestamps)
    {
        if (timestamp_due)
        {
            timestamp_due = false;
            print_time();
#ifdef TRACE
            print_level(_trace_level);
#endif
        }
        if (ch == '\n' || ch == '\r')
            timestamp_due = true;

    }
#endif

}



void vLogCore(int flags, char *file, int line, char *fmt, ...)
{

    if (((1<<flags) & vLogFlags) == 0)
        return;

    int l = strlen(log2st(flags));

    xprintf("%s",log2st(flags)+1);
    for(int i = l; i < maxLogLen(); i++)
        myputchar(' ');

    xprintf("%s:%i  ", parse_filename(file), line);
    va_list arg_ptr;
    va_start (arg_ptr, fmt); /* format string */
    xvprintf (fmt, arg_ptr);
    va_end (arg_ptr);
}

void compareLogFlags(uint32_t old, uint32_t new)
{
    for (int j = 0; j < xCount; j++)
        if ((old & 1<<j) != (new & 1<<j))
        {
            int n = xprintf("%s",log2st(j)+1);
            for(int i = n; i < maxLogLen()-1; i++)
                myputchar(' ');
        
            xprintf("-> %s\n", (new & 1<<j) ? "ON" : "OFF" );
        }
}

#if USE_FULL_ASSERT

/**
   * @brief Reports the name of the source file and the source line number
   * where the assert_param error has occurred.
   * @param file: pointer to the source file name
   * @param line: assert_param error line source number
   * @retval None
   */
void assert_failed(uint8_t *file, uint32_t line)
{
#if defined CY_PINS_Error_LED_H
    Error_LED_Write(LED_ON);
#endif
    while(1)
    {
        xprintf("\nassert failed in: file %s on line %lu\n", file, line);
        CyHalt((uint8) 0u);
//        delay_ms(1000);
//        sec++;
    }
}

#endif

