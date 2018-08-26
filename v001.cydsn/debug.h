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


#ifndef _DEBUG_H
#define _DEBUG_H
    
#include <stdarg.h> 
#include <stdint.h>    
#include <stdbool.h>    
    
#define SHOW_TIMESTAMPS
#define CLEAR_EOL
//#define TRACE
   
#define FEW_LOGS_ON  (1<< xInfo | 1<< xDebug | 1<<xBUG | 1<<xErr)
#ifdef DEBUG    
#define ALL_LOGS_OFF (1<<xBUG | 1<<xErr)
#else
#define ALL_LOGS_OFF 0
#endif    
#define ALL_LOGS_ON  0xffffffff
#define RELEASE_LOGS (1<< xInfo)
#define FLAGS_PROTOCOL 1    
enum
{ // 32 pcs max.
    xInfo        ,
    xBUG         ,    
    xErr         ,
    xBle         ,    
    xDownload    ,    
    xPeakVrbs    ,
    xPeakValue   ,
    xPeakBits    ,
    xProfiler    ,
    xDebug       ,    
    xCount
};
    
//#ifdef DEBUG
     extern uint32_t vLogFlags;
#    define enableLog(v)  { vLogFlags |= 1<<(v); }
#    define disableLog(v) { vLogFlags &= ~(1<<(v));}
//#define vLog(format, args...) xprintf(format, ## args)
//#define vLog(format, args...) xprintf("%s:%i %s() " format, parse_filename(__FILE__), __LINE__, __FUNCTION__, ## args)
//#    define vLog(_f, format, args...) { if ((1<<_f) & vLogFlags) xprintf("{%02i} %s:%i " format, _f, parse_filename(__FILE__), __LINE__, ## args); }
#   define vLog(_f, format, args...) vLogCore(_f, __FILE__, __LINE__, format, ## args)
//#else
//#    define vLog(...) {}
//#    define enableLog(v)  {}
//#    define disableLog(v) {}
//#endif

void debug_stuff(char ch);

#ifdef TRACE

    extern int _trace_level;
#   define F_IN(_f) F_INTRO(_f, "(%s)","")
#   define F_OUT(_f) F_OUTRO(_f, "(%s)","")
#   define F_INTRO(_f, format,...)                                                                                          \
    {                                                                                                                       \
        if ((1<<_f) & vLogFlags)                                                                                            \
        {                                                                                                                   \
            xprintf(" /- %s %s:%i: %s " format "\n", log2st(_f), parse_filename(__FILE__), __LINE__, __FUNCTION__, __VA_ARGS__); \
            _trace_level++;                                                                                                 \
        }                                                                                                                   \
    }                                                                                                                      
#   define F_OUTRO(_f, format,...)                                                                                          \
    {                                                                                                                       \
        if ((1<<_f) & vLogFlags)                                                                                            \
        {                                                                                                                   \
            --_trace_level;                                                                                                 \
            assert(_trace_level >= 0);                                                                                      \
            xprintf(" \\- %s %s:%i: %s " format "\n", log2st(_f), parse_filename(__FILE__), __LINE__, __FUNCTION__,__VA_ARGS__); \
        }                                                                                                                   \
    }
#else
#  define F_IN(...) {}
#  define F_OUT(...) {}
#  define F_INTRO(...) {}
#  define F_OUTRO(...) {}
#endif

//extern UART_HandleTypeDef  huart3;
int maxLogLen(void);
const char *parse_filename(const char *st);
int gotchar(void);
void vLogCore(int flags, char *file, int line, char *fmt, ...);
char *log2st(int t);
void myputchar(unsigned char c);
#ifdef SHOW_TIMESTAMPS
bool show_timestamps(bool val);
#endif    
int print_time(void);
void compareLogFlags(uint32_t old, uint32_t new);


    
#endif    
