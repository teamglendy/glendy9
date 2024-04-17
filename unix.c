#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include "unix.h"

Point
Pt(int x, int y)
{
	Point p = {x, y};
	return p;
}

_Noreturn void
sysfatal(char *fmt, ...)
{
	va_list arg;

	va_start(arg, fmt);
	vfprintf(stderr, fmt, arg);
	va_end(arg);

	exit(127);
}

_Noreturn void
exits(char *s)
{
	if(s == nil)
		exit(0);
	else
		fprintf(stderr, "%s", s);
	exit(127);
}

int
eqpt(Point p, Point q)
{
	return p.x==q.x && p.y==q.y;
}

int
nrand(int n)
{
	return rand() % n;
}
