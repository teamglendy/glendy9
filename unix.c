#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include "unix.h"
#include "util.h"

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
	fprintf(stderr, "\n");
	va_end(arg);

	abort();
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
	return p.x == q.x && p.y == q.y;
}

int
nrand(int n)
{
	return rand() % n;
}

/*
 * i'm well aware of printf and it's friends
 * but i had to prefer one (Plan 9) interface over other one (POSIX/ANSI)
 */
int
vsprint(char *out, char *fmt, va_list arg)
{
	int n;
	n = vsprintf(out, fmt, arg);
	return n;
}

int
sprint(char *out, char *fmt, ...)
{
	int n;
	va_list arg;
	va_start(arg, fmt);
	
	n = vsprint(out, fmt, arg);
	va_end(arg);
	return n;
}

int
vfprint(int fd, char *fmt, va_list arg)
{
	char s[512];
	int n;

	n = vsprint(s, fmt, arg);
	if(write(fd, s, n) < n)
		perror("couldn't write");
	return n;
}

int
fprint(int fd, char *fmt, ...)
{
	int n;
	va_list arg;
	va_start(arg, fmt);
	
	n = vfprint(fd, fmt, arg);
	va_end(arg);
	return n;
}

int
print(char *fmt, ...)
{
	int n;
	va_list args;

	va_start(args, fmt);
	n = vfprint(1, fmt, args);
	va_end(args);
	return n;
}


