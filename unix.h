/* this code is uglier than what it should be */
#pragma once
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>

/* uncomment if your compiler doesn't support it (ANSI, C99)
#define _Noreturn
*/

#define nil NULL

typedef
struct Point
{
	int x;
	int y;
}Point;

Point Pt(int x, int y);
_Noreturn void sysfatal(char *fmt, ...);
_Noreturn void exits(char *s);
int eqpt(Point p, Point q);
int nrand(int n);

/* fmt */
int vsprint(char *out, char *fmt, va_list arg);
int sprint(char *out, char *fmt, ...);
int vfprint(int fd, char *fmt, va_list arg);
int fprint(int fd, char *fmt, ...);
int print(char *fmt, ...);