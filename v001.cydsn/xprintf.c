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


#include <stdarg.h>
#include "xprintf.h"


#if _USE_XFUNC_OUT
#include <stdarg.h>
void (*xfunc_out)(unsigned char);	/* Pointer to the output stream */
static char *outptr;

/*----------------------------------------------*/
/* Put a character                              */
/*----------------------------------------------*/

void xputc (char c)
{
	if (_CR_CRLF && c == '\n') xputc('\r');		/* CR -> CRLF */

	if (outptr) {
		*outptr++ = (unsigned char)c;
		return;
	}

	if (xfunc_out) xfunc_out((unsigned char)c);
}



/*----------------------------------------------*/
/* Put a null-terminated string                 */
/*----------------------------------------------*/

unsigned long xnputs (					/* Put a string to the default device */
    unsigned long len,
	const char* str				/* Pointer to the string */
)
{
    unsigned long ret = 0;
	while (*str && len > 0)
    {
        len--;
        ret++;
		xputc(*str++);
    }
    return ret;
}


unsigned long xfnputs (					/* Put a string to the specified device */
	void(*func)(unsigned char),	/* Pointer to the output function */
    unsigned long len,    
	const char*	str				/* Pointer to the string */
)
{
    unsigned long ret = 0;    
	void (*pf)(unsigned char);


	pf = xfunc_out;		/* Save current output device */
	xfunc_out = func;	/* Switch output to specified device */
	while (*str && len > 0)		/* Put the string */
    {
        len--;
        ret++;        
		xputc(*str++);
    }
	xfunc_out = pf;		/* Restore output device */
    return ret;    
}



/*----------------------------------------------*/
/* Formatted string output                      */
/*----------------------------------------------*/
/*  xprintf("%d", 1234);			"1234"
    xprintf("%6d,%3d%%", -200, 5);	"  -200,  5%"
    xprintf("%-6u", 100);			"100   "
    xprintf("%ld", 12345678L);		"12345678"
    xprintf("%04x", 0xA3);			"00a3"
    xprintf("%08LX", 0x123ABC);		"00123ABC"
    xprintf("%016b", 0x550F);		"0101010100001111"
    xprintf("%s", "String");		"String"
    xprintf("%-4s", "abc");			"abc "
    xprintf("%4s", "abc");			" abc"
    xprintf("%c", 'a');				"a"
    xprintf("%f", 10.0);            <xprintf lacks floating point support>
*/

unsigned long xvnprintf (
    unsigned long len,
	const char*	fmt,	/* Pointer to the format string */
	va_list arp			/* Pointer to arguments */
)
{
	unsigned int r = 10, i, j, w, f;
    unsigned long rl = 0;
	unsigned long v;
	char s[16], c, d, *p;


	for (;;) {
        if (len == 0) return rl;
		c = *fmt++;					/* Get a char */
		if (!c) break;				/* End of format? */
		if (c != '%') {				/* Pass through it if not a % sequense */
			xputc(c); 
            len--; rl++; if (len == 0) return rl;
            continue;
		}
		f = 0;
		c = *fmt++;					/* Get first char of the sequense */
		if (c == '0') {				/* Flag: '0' padded */
			f = 1; c = *fmt++;
		} else {
			if (c == '-') {			/* Flag: left justified */
				f = 2; c = *fmt++;
			}
		}
		for (w = 0; c >= '0' && c <= '9'; c = *fmt++)	/* Minimum width */
			w = w * 10 + c - '0';
		if (c == 'l' || c == 'L') {	/* Prefix: Size is long int */
			f |= 4; c = *fmt++;
		}
		if (!c) break;				/* End of format? */
		d = c;
		if (d >= 'a') d -= 0x20;
		switch (d) {				/* Type is... */
		case 'S' :					/* String */
            {
                unsigned long tmp;
    			p = va_arg(arp, char*);
    			for (j = 0; p[j]; j++) ;
    			while (!(f & 2) && j++ < w) { xputc(' '); len--; rl++; if (len == 0) return rl;}
    			tmp = xnputs(len, p);
                len -= tmp;
                rl += tmp;
                if (len == 0) return rl;
    			while (j++ < w) { xputc(' '); len--; rl++; if (len == 0) return rl;}
    			continue;
            }
		case 'C' :					/* Character */
			xputc((char)va_arg(arp, int)); 
            len--; rl++; if (len == 0) break;
            continue;
		case 'B' :					/* Binary */
			r = 2; break;
		case 'O' :					/* Octal */
			r = 8; break;
		case 'I' :					/* Signed decimal */
		case 'D' :					/* Signed decimal */
		case 'U' :					/* Unsigned decimal */
			r = 10; break;
		case 'X' :					/* Hexdecimal */
			r = 16; break;
		case 'Z' :					/* Hexdecimal */
			r = 36; break;
		default:					/* Unknown type (passthrough) */            
			xputc(c); 
            len--; rl++; if (len == 0) break;
            continue;
		}

		/* Get an argument and put it in numeral */
		v = (f & 4) ? va_arg(arp, long) : ((d == 'D' || d == 'I') ? (long)va_arg(arp, int) : (long)va_arg(arp, unsigned int));
		if ((d == 'D' || d == 'I') && (v & 0x80000000)) {
			v = 0 - v;
			f |= 8;
		}
		i = 0;
		do {
			d = (char)(v % r); v /= r;
			if (d > 9) d += (c == 'x') ? 0x27 : 0x07;
			s[i++] = d + '0';
		} while (v && i < sizeof(s));
		if (f & 8) s[i++] = '-';
		j = i; d = (f & 1) ? '0' : ' ';
		while (!(f & 2) && j++ < w) { xputc(d); len--; rl++; if (len == 0) return rl;}
		do {
            xputc(s[--i]); 
            len--; rl++; if (len == 0) return rl;
        }while(i);
		while (j++ < w) { xputc(' '); len--; rl++; if (len == 0) return rl;}
	}
    
    return rl;
}


unsigned long xprintf (			/* Put a formatted string to the default device */
	const char*	fmt,	/* Pointer to the format string */
	...					/* Optional arguments */
)
{
    unsigned long ret;
	va_list arp;

	va_start(arp, fmt);
	ret = xvnprintf(0xffffffff, fmt, arp);
	va_end(arp);
    
    return ret;
}


unsigned long xsnprintf (			/* Put a formatted string to the memory */
	char* buff,			/* Pointer to the output buffer */
    unsigned long len,
	const char*	fmt,	/* Pointer to the format string */
	...					/* Optional arguments */
)
{
        unsigned long ret;
	va_list arp;


	outptr = buff;		/* Switch destination for memory */

	va_start(arp, fmt);
	ret = xvnprintf(len, fmt, arp);
	va_end(arp);

	*outptr = 0;		/* Terminate output string with a \0 */
	outptr = 0;			/* Switch destination for device */
    
    return ret;
}


unsigned long xfnprintf (					/* Put a formatted string to the specified device */
	void(*func)(unsigned char),	/* Pointer to the output function */
    unsigned long len,    
	const char*	fmt,			/* Pointer to the format string */
	...							/* Optional arguments */
)
{
    unsigned long ret;    
	va_list arp;
	void (*pf)(unsigned char);


	pf = xfunc_out;		/* Save current output device */
	xfunc_out = func;	/* Switch output to specified device */

	va_start(arp, fmt);
	ret = xvnprintf(len, fmt, arp);    
	va_end(arp);

	xfunc_out = pf;		/* Restore output device */
    return ret;
}



/*----------------------------------------------*/
/* Dump a line of binary dump                   */
/*----------------------------------------------*/

void put_dump (
	const void* buff,		/* Pointer to the array to be dumped */
	unsigned long addr,		/* Heading address value */
	int len,				/* Number of items to be dumped */
	int width				/* Size of the items (DF_CHAR, DF_SHORT, DF_LONG) */
)
{
	int i;
	const unsigned char *bp;
	const unsigned short *sp;
	const unsigned long *lp;


	xprintf("%08lX ", addr);		/* address */

	switch (width) {
	case DW_CHAR:
		bp = buff;
		for (i = 0; i < len; i++)		/* Hexdecimal dump */
			xprintf(" %02X", bp[i]);
		xputc(' ');
		for (i = 0; i < len; i++)		/* ASCII dump */
			xputc((bp[i] >= ' ' && bp[i] <= '~') ? bp[i] : '.');
		break;
	case DW_SHORT:
		sp = buff;
		do								/* Hexdecimal dump */
			xprintf(" %04X", *sp++);
		while (--len);
		break;
	case DW_LONG:
		lp = buff;
		do								/* Hexdecimal dump */
			xprintf(" %08LX", *lp++);
		while (--len);
		break;
	}

	xputc('\n');
}

#endif /* _USE_XFUNC_OUT */



#if _USE_XFUNC_IN
unsigned char (*xfunc_in)(void);	/* Pointer to the input stream */

/*----------------------------------------------*/
/* Get a line from the input                    */
/*----------------------------------------------*/

int xgets (		/* 0:End of stream, 1:A line arrived */
	char* buff,	/* Pointer to the buffer */
	int len		/* Buffer length */
)
{
	int c, i;


	if (!xfunc_in) return 0;		/* No input function specified */

	i = 0;
	for (;;) {
		c = xfunc_in();				/* Get a char from the incoming stream */
		if (!c) return 0;			/* End of stream? */
		if (c == '\r') break;		/* End of line? */
		if (c == '\b' && i) {		/* Back space? */
			i--;
			if (_LINE_ECHO) xputc(c);
			continue;
		}
		if (c >= ' ' && i < len - 1) {	/* Visible chars */
			buff[i++] = c;
			if (_LINE_ECHO) xputc(c);
		}
	}
	buff[i] = 0;	/* Terminate with a \0 */
	if (_LINE_ECHO) xputc('\n');
	return 1;
}


int xfgets (	/* 0:End of stream, 1:A line arrived */
	unsigned char (*func)(void),	/* Pointer to the input stream function */
	char* buff,	/* Pointer to the buffer */
	int len		/* Buffer length */
)
{
	unsigned char (*pf)(void);
	int n;


	pf = xfunc_in;			/* Save current input device */
	xfunc_in = func;		/* Switch input to specified device */
	n = xgets(buff, len);	/* Get a line */
	xfunc_in = pf;			/* Restore input device */

	return n;
}


/*----------------------------------------------*/
/* Get a value of the string                    */
/*----------------------------------------------*/
/*	"123 -5   0x3ff 0b1111 0377  w "
	    ^                           1st call returns 123 and next ptr
	       ^                        2nd call returns -5 and next ptr
                   ^                3rd call returns 1023 and next ptr
                          ^         4th call returns 15 and next ptr
                               ^    5th call returns 255 and next ptr
                                  ^ 6th call fails and returns 0
*/

int xatoi (			/* 0:Failed, 1:Successful */
	char **str,		/* Pointer to pointer to the string */
	long *res		/* Pointer to the valiable to store the value */
)
{
	unsigned long val;
	unsigned char c, r, s = 0;


	*res = 0;

	while ((c = **str) == ' ') (*str)++;	/* Skip leading spaces */

	if (c == '-') {		/* negative? */
		s = 1;
		c = *(++(*str));
	}

	if (c == '0') {
		c = *(++(*str));
		switch (c) {
		case 'x':		/* hexdecimal */
			r = 16; c = *(++(*str));
			break;
		case 'b':		/* binary */
			r = 2; c = *(++(*str));
			break;
		default:
			if (c <= ' ') return 1;	/* single zero */
			if (c < '0' || c > '9') return 0;	/* invalid char */
			r = 8;		/* octal */
		}
	} else {
		if (c < '0' || c > '9') return 0;	/* EOL or invalid char */
		r = 10;			/* decimal */
	}

	val = 0;
	while (c > ' ') {
		if (c >= 'a') c -= 0x20;
		c -= '0';
		if (c >= 17) {
			c -= 7;
			if (c <= 9) return 0;	/* invalid char */
		}
		if (c >= r) return 0;		/* invalid char for current radix */
		val = val * r + c;
		c = *(++(*str));
	}
	if (s) val = 0 - val;			/* apply sign if needed */

	*res = val;
	return 1;
}

#endif /* _USE_XFUNC_IN */
