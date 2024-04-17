/* this code is uglier than what it should be */
#pragma once
#include <string.h>
#include <stdio.h> /* replace with plan 9's print? */

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
