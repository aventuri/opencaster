/*
 * util.c
 */

/*
 * Copyright (C) 2005, Simon Kilvington
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include "utils.h"
#include "carousel.h"

static int _verbose = 0;

void increase_verbose() {
	_verbose++;
}

void set_verbose(int level) {
	_verbose = level;
}

/*
 * move str to the next non-white space character (or the end of the string)
 */

char *
skip_ws(char *str)
{
	if(str == NULL)
		return NULL;

	while(*str != '\0' && isspace((int) *str))
		str ++;

	return str;
}

/*
 * returns a single ASCII char for the values 0-15
 */

char
hex_digit(uint8_t val)
{
	/* assert */
	if(val > 0xf)
		fatal("Not a single hex digit");

	return (val < 10) ? '0' + val : 'a' + (val - 10);
}

/*
 * I don't want to double the size of my code just to deal with malloc failures
 * if you've run out of memory you're fscked anyway, me trying to recover is not gonna help...
 */

void *
safe_malloc(size_t nbytes)
{
	void *buf;

	if((buf = malloc(nbytes)) == NULL)
		fatal("Out of memory");

	return buf;
}

/*
 * safe_realloc(NULL, n) == safe_malloc(n)
 */

void *
safe_realloc(void *oldbuf, size_t nbytes)
{
	void *newbuf;

	if(oldbuf == NULL)
		return safe_malloc(nbytes);

	if((newbuf = realloc(oldbuf, nbytes)) == NULL)
		fatal("Out of memory");

	return newbuf;
}

/*
 * safe_free(NULL) is okay
 */

void
safe_free(void *buf)
{
	if(buf != NULL)
		free(buf);

	return;
}

void
error(char *message, ...)
{
	va_list ap;

	va_start(ap, message);
	vfprintf(stderr, message, ap);
	fprintf(stderr, "\n");
	va_end(ap);

	return;
}

void
fatal(char *message, ...)
{
	va_list ap;

	va_start(ap, message);
	vfprintf(stderr, message, ap);
	fprintf(stderr, "\n");
	va_end(ap);

}


/* number of bytes per line */
#define HEXDUMP_WIDTH	16

void
hexdump(unsigned char *data, size_t nbytes)
{
	size_t nout;
	int i;

	nout = 0;
	while(nout < nbytes)
	{
		/* byte offset at start of line */
		if((nout % HEXDUMP_WIDTH) == 0)
			printf("0x%.8x  ", nout);
		/* the byte value in hex */
		printf("%.2x ", data[nout]);
		/* the ascii equivalent at the end of the line */
		if((nout % HEXDUMP_WIDTH) == (HEXDUMP_WIDTH - 1))
		{
			printf(" ");
			for(i=HEXDUMP_WIDTH-1; i>=0; i--)
				printf("%c", isprint(data[nout - i]) ? data[nout - i] : '.');
			printf("\n");
		}
		nout ++;
	}

	/* the ascii equivalent if we haven't just done it */
	if((nout % HEXDUMP_WIDTH) != 0)
	{
		/* pad to the start of the ascii equivalent */
		for(i=(nout % HEXDUMP_WIDTH); i<HEXDUMP_WIDTH; i++)
			printf("   ");
		printf(" ");
		/* print the ascii equivalent */
		nout --;
		for(i=(nout % HEXDUMP_WIDTH); i>=0; i--)
			printf("%c", isprint(data[nout - i]) ? data[nout - i] : '.');
		printf("\n");
	}

	return;
}


void
verbose(char *message, ...)
{
	va_list ap;

	if(_verbose > 0)
	{
	        va_start(ap, message);
	        vprintf(message, ap);
		printf("\n");
	        va_end(ap);
	}

	return;
}

void
vverbose(char *message, ...)
{
	va_list ap;

	if(_verbose > 1)
	{
	        va_start(ap, message);
	        vprintf(message, ap);
		printf("\n");
	        va_end(ap);
	}

	return;
}

void
vhexdump(unsigned char *data, size_t nbytes)
{
	if(_verbose > 2)
		hexdump(data, nbytes);

	return;
}


int
timeval_subtract (struct timeval *result,struct timeval *x,struct timeval *y)
{
	/* Perform the carry for the later subtraction by updating y. */
	if (x->tv_usec < y->tv_usec) {
		int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
		y->tv_usec -= 1000000 * nsec;
		y->tv_sec += nsec;
	}
	if (x->tv_usec - y->tv_usec > 1000000) {
		int nsec = (x->tv_usec - y->tv_usec) / 1000000;
		y->tv_usec += 1000000 * nsec;
		y->tv_sec -= nsec;
	}
	
	/* Compute the time remaining to wait. tv_usec is certainly positive. */
	result->tv_sec = x->tv_sec - y->tv_sec;
	result->tv_usec = x->tv_usec - y->tv_usec;
	
	/* Return 1 if result is negative. */
	return x->tv_sec < y->tv_sec;
}				

