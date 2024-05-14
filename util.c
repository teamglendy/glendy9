#ifdef unix
#include "unix.h"
#else
#include <u.h>
#include <libc.h>
#include <draw.h>
#endif

#include "engine.h"

int
isnum(char *s, unsigned int n)
{
	if(strlen(s) < n)
		n = strlen(s);
	
	for(int i = 0 ; i < n ; i++)
	{
		if(s[i] > '9' || s[i] < '0')
			return 0;
	}
	return 1;
}

int
parsemove(char *s)
{
	int d;
	
	if(strcmp(s, "NE") == 0)
		d = NE;
	else if(strcmp(s, "E") == 0)
		d =  E;
	else if(strcmp(s, "SE") == 0)
		d = SE;
	else if(strcmp(s, "W") == 0)
		d =  W;
	else if(strcmp(s, "SW") == 0)
		d = SW;
	else if(strcmp(s, "NW") == 0)
		d = NW;
	else
		sysfatal("parsemove(): invalid direction");

	return d;
}

char*
dirtostr(int dir)
{
	switch(dir)
	{
		case NE:
			return "NE";
		case E:
			return "E";
		case SE:
			return "SE";
		case SW:
			return "SW";
		case W:
			return "W";
		case NW:
			return "NW";
		default:
			return nil;
	}
}

/* xx yy\0
static Point
parseput(char *s)
{
	int x, y;
	int len;
	
	x = atoi(s);
	s = strchr(s, ' ');
	if(end == nil)
	{
		dprint("parseput(): end nil\n");
		sysfatal("parseput(): incomplete line");
	}
	y = atoi(s);
	return Pt(x, y);
}
*/

Point
parseput(char *x, char *y)
{
	if(isnum(x, 2) != 1 && isnum(y, 2) != 1)
		sysfatal("parseput(): input isnt a number?");
	
	return Pt(atoi(x), atoi(y));
}

int
dprint(char *fmt, ...)
{
	va_list va;
	int n;

	if(!debug)
		return 0;
	
	va_start(va, fmt);
	n = vfprint(2, fmt, va);
	va_end(va);
	return n;
}

void*
emalloc(unsigned long n)
{
	void *p;

	p = malloc(n);
	if(p == nil)
		sysfatal("mallocz: %r");
	
	return p;	
}
