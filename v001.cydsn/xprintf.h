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


#ifndef _STRFUNC
#define _STRFUNC

#define _USE_XFUNC_OUT	1	/* 1: Use output functions */
#define	_CR_CRLF		0	/* 1: Convert \n ==> \r\n in the output char */

#define _USE_XFUNC_IN	0	/* 1: Use input function */
#define	_LINE_ECHO		1	/* 1: Echo back input chars in xgets function */

#if _USE_XFUNC_OUT
       
#define xdev_out(func) xfunc_out = (void(*)(unsigned char))(func)
extern void (*xfunc_out)(unsigned char);
void xputc (char c);
unsigned long xnputs (unsigned long len, const char* str);
unsigned long xfnputs (void (*func)(unsigned char), unsigned long len, const char* str);
unsigned long xprintf (const char* fmt, ...);
unsigned long xsnprintf (char* buff, unsigned long len, const char* fmt, ...);
unsigned long xfnprintf (void (*func)(unsigned char), unsigned long len, const char*	fmt, ...);    
unsigned long xvnprintf (unsigned long len, const char*	fmt, va_list arp);
void put_dump (const void* buff, unsigned long addr, int len, int width);

#define xvprintf(args...) xvnprintf(0xffffffff, ## args)
#define xsprintf(buf, args...) xsnprintf(buf, 0xffffffff, ## args)

#define DW_CHAR		sizeof(char)
#define DW_SHORT	sizeof(short)
#define DW_LONG		sizeof(long)
#endif

#if _USE_XFUNC_IN
#define xdev_in(func) xfunc_in = (unsigned char(*)(void))(func)
extern unsigned char (*xfunc_in)(void);
int xgets (char* buff, int len);
int xfgets (unsigned char (*func)(void), char* buff, int len);
int xatoi (char** str, long* res);
#endif

#endif
