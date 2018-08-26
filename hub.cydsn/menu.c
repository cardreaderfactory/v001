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
#include "stdbool.h"
#include "stdint.h"
#include "xprintf.h"
#include "common.h"
#include "debug.h"
#include "device.h"
#include "bluetooth.h"
#include "debug.h"

#define print_info(args...) {xsprintf(print_buf, ## args); print_line(MENU_EDGE, ' ', INFO_SIZE, print_buf); }
#define MENU_EDGE '|'
#define MENU_TOP  '='
#define MENU_SUB  '-'
#define MENU_SIZE 50

typedef struct
{
    char        key;              // key to press and show on the screen
    uint8_t     level;
    uint8_t     bit;
    uint32_t   *flags;
    const char *comment;           // description shown to the user
    void      (*func)(void);       // function to execute
} menu_display_t;

uint32_t display_keys = 1;
//volatile bool cancelDl = true;


void disconnect         (void);
void menuReadStats      (void);
void menuSetTime        (void);
void eraseMem           (void);
void peerReboot         (void);
void advertising        (void);
void logIn              (void);
void changePass         (void);
void changeKey          (void);
void changeName         (void);
void upgrade            (void);
void downloadUsedMem    (void);
void downloadAll        (void);
void menuCancelDownload (void);
void switchAllFlags     (void);
void writeFlags         (void);
void help               (void);
void reboot             (void);


const menu_display_t mm[] = {
//    Key  , Level, Bit to toggle , Toggle variable    , Comment                            , Function
    { 0   , 0  , 0             , NULL               , "BLE"                             , NULL                  },
    { '.' , 1  , 0             , NULL               , "Disconnect"                      , disconnect            },
                                                                                                                
    { 0   , 0  , 0             , NULL               , "Commands"                        , NULL                  },
    { 's' , 1  , 0             , NULL               , "Stats"                           , menuReadStats         }, 
    { 't' , 1  , 0             , NULL               , "Time"                            , menuSetTime           }, 
    { 'e' , 1  , 0             , NULL               , "Erase Memory"                    , eraseMem              },    
    { 'R' , 1  , 0             , NULL               , "Reboot remote device"            , peerReboot            }, 
    
    { 0   , 0  , 0             , NULL               , "Commands in development"         , NULL                  },
    { '!' , 1  , 0             , NULL               , "Start advertising"               , advertising           }, 
    { 'l' , 1  , 0             , NULL               , "Log In"                          , logIn                 }, 
    { 'p' , 1  , 0             , NULL               , "Password"                        , changePass            }, 
    { 'k' , 1  , 0             , NULL               , "Key"                             , changeKey             }, 
    { 'n' , 1  , 0             , NULL               , "Name"                            , changeName            }, 
    { 'u' , 1  , 0             , NULL               , "Upgrade"                         , upgrade               }, 
                                                                                                                
    { 0   , 0  , 0             , NULL               , "Transfer"                        , NULL                  },
    { 'd' , 1  , 0             , NULL               , "Download Used Memory"            , downloadUsedMem       },
    { 'D' , 1  , 0             , NULL               , "Download All"                    , downloadAll           },
    { 'c' , 1  , 0             , NULL               , "Cancel Download"                 , menuCancelDownload    },    
                                                                                        
#ifdef DEBUG
    { 0   , 0  , 0             , NULL               , "Messages"                        , NULL                  },    
    { '@' , 1  , 0             , NULL               , "All"                             , switchAllFlags        },
    { '#' , 1  , 0             , &log_sync          , "Sync logs"                       , writeFlags            },
    { '1' , 1  , xInfo         , &vLogFlags         , __stringnify(xInfo     )          , NULL                  },
    { '2' , 1  , xDebug        , &vLogFlags         , __stringnify(xDebug    )          , NULL                  },
    { '3' , 1  , xBle          , &vLogFlags         , __stringnify(xBle      )          , NULL                  },
    { '4' , 1  , xDownload     , &vLogFlags         , __stringnify(xDownload )          , NULL                  },
    { '5' , 1  , xPeakBits     , &vLogFlags         , __stringnify(xPeakBits )          , NULL                  },
    { '6' , 1  , xPeakValue    , &vLogFlags         , __stringnify(xPeakValue)          , NULL                  },
    { '7' , 1  , xPeakVrbs     , &vLogFlags         , __stringnify(xPeakVrbs )          , NULL                  },
    { '8' , 1  , xProfiler     , &vLogFlags         , __stringnify(xProfiler )          , NULL                  },
    { '9' , 1  , xBUG          , &vLogFlags         , __stringnify(xBUG      )          , NULL                  },
    { '0' , 1  , xErr          , &vLogFlags         , __stringnify(xErr      )          , NULL                  },
#endif    
    
    { 0   , 0  , 0             , NULL               , "Others"                          , NULL                  },    
    { '*' , 1  , 0             , &display_keys      , "Display menu keys"               , NULL                  },
    { '?' , 1  , 0             , NULL               , "Help"                            , help                  },
    { 'r' , 1  , 0             , NULL               , "Reboot"                          , reboot                },
};

void print_break_line(int ch, int cnt)
{
    for (int i = 0; i < cnt; i++) myputchar(ch);
    myputchar('\n');
}

void print_line(char edge_char, char fill_char, int cnt, char *st)
{
    myputchar(edge_char);
    myputchar(fill_char);
    xprintf("%s",st);
    for (int i = cnt - strlen(st) - 4; i > 0; i--) myputchar(fill_char);
    myputchar(edge_char);
    myputchar('\n');
}

void reboot(void)
{
    vLog(xInfo, "rebooting...\n");
    UART_UartPutChar('\n');
    while (UART_SpiUartGetTxBufferSize() != 0);
    CySoftwareReset();
}

void disconnect(void)
{
    vLog(xBle, "disconnecting...\n");
    CyBle_GapDisconnect(cyBle_connHandle.bdHandle);
}

void downloadAll(void)
{
    vLog(xDebug, "Downloading all memory\n");    
    download(0, 0xfffff);
//    int err = 0;
//    cancelDl = false;
//    for (int i = 17900; i < 19000; i++)
//    {
//        vLog(xDebug, "Downloading all memory from offset %i to %i\n", i, i+550);    
//        if (download(i, i+550) == 0)
//            err++;
//        
//        if (err > 10)
//            break;
//        if (cancelDl)
//            break;
//    }    
}

void menuCancelDownload(void)
{
//    cancelDl = true;
//    vLog(xDebug, "%s\n", __FUNCTION__);
    
    if (isDownloading())    
        cancelDownload("cancelled by user");    
}
        
void downloadUsedMem(void)
{
    menuCancelDownload();
//    if (isDownloading())    
//        cancelDownload("cancelled by user");
        
    memset(&stats, 0, sizeof(stats));
    pending_stats_read = 1;
    if (readStats())
    {
        //vLog(xDebug, "pending_stats_read = %i\n", pending_stats_read);
        if (yield(500, &pending_stats_read))
        {
            if (stats.memoryUsed > 0)
            {
                vLog(xDownload, "downloading %i bytes...\n", stats.memoryUsed);
                download(0, stats.memoryUsed);
            }
            else
            {
                vLog(xDownload, "memory is emtpy\n", stats.memoryUsed);
            }
        }
        else
        {
            vLog(xDownload, "failed to read memory used\n");
        }
        //vLog(xDebug, "pending_stats_read = %i\n", pending_stats_read);
    }            
}

void advertising(void)
{
    menuCancelDownload();
    vLog(xInfo, "%s() not implemented\n", __FUNCTION__ );   
}

void logIn(void)
{
    menuCancelDownload();
    
    char *st = "login name";
    sendCommand(CYBLE_MSRVXXX_LOGIN_CHAR_HANDLE, st, strlen(st));
}

void changePass(void)
{
    menuCancelDownload();
    char *st = "my password";
    sendCommand(CYBLE_MSRVXXX_SETPASS_CHAR_HANDLE, st, strlen(st));
}
void changeKey(void)
{
    menuCancelDownload();
    char *st = "1234567890abcdef";
    sendCommand(CYBLE_MSRVXXX_SETKEY_CHAR_HANDLE, st, 16);
}
void changeName(void)
{
    menuCancelDownload();
    char *st = "new device name";
    sendCommand(CYBLE_MSRVXXX_SETNAME_CHAR_HANDLE, st, strlen(st));
}

void upgrade(void)
{
    menuCancelDownload();
    char *st = "device upgrade code";
    sendCommand(CYBLE_MSRVXXX_UPGRADE_CHAR_HANDLE, st, strlen(st));
}

void menuSetTime(void)
{
    menuCancelDownload();
    if (CySysWdtGetCount(0) < 31500-5000)
        vLog(xInfo, "waiting for time sync ...\n");
    while (CySysWdtGetCount(0) != 31500); // assuming latency of 1268 ticks (39ms) ... 
    uint32_t time = timestamp+1;
    if (sendCommand(CYBLE_MSRVXXX_SETTIME_CHAR_HANDLE, &time, 4))
    {
        vLog(xInfo, "Current timestamp was sent to the device\n");
    }
}

void eraseMem(void)
{
    menuCancelDownload();
    
    bool all = true;
    sendCommand(CYBLE_MSRVXXX_ERASE_CHAR_HANDLE, &all, sizeof(all));
}

void peerReboot(void)
{
    sendCommand(CYBLE_MSRVXXX_REBOOT_CHAR_HANDLE, NULL, 0);
   
}


void menuReadStats(void)
{
    menuCancelDownload();
    memset(&stats, 0, sizeof(stats));    
    if (readStats())
    {
        if (yield(500, &pending_stats_read))
        {
//            showStats(xInfo, &stats);
        }
        else
        {
            vLog(xInfo, "failed to read stats\n");
        }    
    }
    else
    {
        vLog(xInfo, "failed to  send readStats()\n");
    }
}
void writeFlags(void)
{
    writeLogFlags(vLogFlags);
}

void switchAllFlags(void)
{    
    uint32_t old = vLogFlags;
    if ((vLogFlags & (~ALL_LOGS_OFF)) != 0) 
        vLogFlags = ALL_LOGS_OFF;
    else
        vLogFlags = ALL_LOGS_ON;

    compareLogFlags(old, vLogFlags);
    
    if (log_sync)
        writeFlags();
   
}

            

#if 0
char tolower(char ch)
{
    if (ch >= 'A' && ch <= 'Z')
        return ch + ('a' - 'A');
    else
        return ch;
}

char toupper(char ch)
{
    if (ch >= 'a' && ch <= 'z')
        return ch - ('a' - 'A');
    else
        return ch;
}
#endif

int snprint_comment(char *buf, int avl, const char *comment, int key)
{
    if (key == 0 || !display_keys)
    {
        return xsnprintf(buf, avl, "%s",comment);
    }

    int cnt = 0;
    int ch;
    bool u = false;

    while (*comment != 0 && avl > 0)
    {
        ch = *comment;
        if (!u && tolower(ch) == tolower(key) && avl >= 3)
        {
            *buf++ = '(';
            *buf++ = key;
            *buf = ')';
            avl -= 3;
            u = true;
        }
        else
        {
            avl--;
            *buf = *comment;
        }
        buf++;
        comment++;
        cnt++;
    }
    *buf = 0;

    return cnt;
}

void check_duplicate_menu_keys()
{
    bool d = false;
    bool display = false;
    bool dsp[ARRAY_SIZE(mm)];
    
    memset(dsp, 0, sizeof(dsp));    
    for (int i = 0; i < ARRAY_SIZE(mm); i++)
    {
        if (dsp[i])
            continue;
        dsp[i] = true;
        d = false;
        for (int j = i+1; j < ARRAY_SIZE(mm); j++)
            if (mm[i].key != 0 && mm[i].key == mm[j].key)
            {
                dsp[j] = true;
                if (!display)
                {
                    myputchar('\n');
                    display = true;
                }
                if (!d)
                    xprintf("Warning, same key for: \n\t%c) %s\n\t%c) %s\n", mm[i].key, mm[i].comment, mm[j].key, mm[j].comment);
                else
                    xprintf("\t%c) %s\n", mm[j].key, mm[j].comment);
                    
                d = true;
            }
    }

    if (display)
        xprintf("\n");
}

void help(void)
{
    char buf[48] = {0};
    char *p;
    uint32_t avl;
    uint32_t l;

    xsnprintf(buf, sizeof(buf) - 1, "Main Menu - " DEVICE_NAME " v" SOFTWARE_VERSION);
//    printf("sizeof mm= %i\n", sizeof(mm));
    print_line(MENU_EDGE, MENU_TOP, MENU_SIZE, "");
    print_line(MENU_EDGE, ' ',  MENU_SIZE, buf);
    print_line(MENU_EDGE, MENU_SUB,  MENU_SIZE, "");

    for (int i = 0; i < ARRAY_SIZE(mm); i++)
    {
        p = buf;
        avl = sizeof(buf) - 1;
 //       assert(mm[i].level < 2);
        for (int j = 0; j < mm[i].level; j++) { l = xsnprintf(p, avl, "  "); avl -= l; p+=l;}
        
        if (avl > 0 && mm[i].key != 0)
        { 
            l = xsnprintf(p, avl, "%c) ", mm[i].key);
            avl -= l; p+=l;
        }
        if (avl > 0 && mm[i].flags != NULL)
        {
            if (*(uint32_t*)(mm[i].flags) & (1<<mm[i].bit))
                l = xsnprintf(p, avl, "Turn OFF ");
            else
                l = xsnprintf(p, avl, "Turn ON ");
            avl -= l; p+=l;
        }
        if (avl > 0)
        {
            l = snprint_comment(p, avl, mm[i].comment, mm[i].key);
            avl -= l; p+=l;
        }
        
        print_line(MENU_EDGE, ' ', MENU_SIZE, buf);
        assert(p <= buf + sizeof(buf));

//        if ( i+1 < ARRAY_SIZE(mm) && mm[i].level > mm[i+1].level)
//            print_line(MARK, ' ', MENU_SIZE, "");

    }
    print_line(MENU_EDGE, MENU_TOP, MENU_SIZE, "");
    print_line(MENU_EDGE, ' ',  MENU_SIZE, BUILD_TYPE " version build on " __DATE__ " " __TIME__ "");
    print_line(MENU_EDGE, MENU_SUB,  MENU_SIZE, "");

}

void menu(void)
{

    int ch = UART_UartGetChar();
    if (ch == 0)
        return;
//    printf("ch=%i\n", ch);

    for (int i = 0; i < ARRAY_SIZE(mm); i++)
    {
        if (ch != mm[i].key)
            continue;

        if (mm[i].flags != NULL)
        {
            bool b;
            uint32_t old = *mm[i].flags;
            
            *mm[i].flags ^=  1<<mm[i].bit;
            b = (*mm[i].flags & 1<<mm[i].bit) != 0;
            
            if (mm[i].flags == &vLogFlags)
            {
                if (log_sync)
                    writeFlags();
                    
                compareLogFlags(old, vLogFlags);
            }
            else
            {
                xprintf("%s --> %s\n", mm[i].comment, b ? "ON" : "OFF" );    
            }
        }

        if (mm[i].func != NULL)
            mm[i].func();

        break;
    }
}
