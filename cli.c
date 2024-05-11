/*
 * cli glendy, based on mitrchoviski's gui for plan 9
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include "unix.h"
#include "engine.h"

char *gface[2] = {"☺", "☹"}; /* glenda's face(es) */
int debug = 0;

void
drawlevel(void)
{
	int x, y;
	
	/* prints first row, this assumes SzX = SzY, is there a better way? */
	print("T%2d|", turn);
	for(int i = 0 ; i < SzX ; i++)
		print("%2d |", i);

	print("\n");

	for(x = 0; x < SzX; x++)
	{
		for(int i = 0 ; i < SzY+1 ; i++)
			print("----");
		print(x % 2 ? "\\" : "/");
		print("\n");

		/* show column number and have a zig-zag effect */
		print("%2d%s |", x, x % 2 ? "  " : "");
		// print("%2d |", x);

		for(y = 0; y < SzY; y++)
		{
			/* it's [y][x], not [x][y] */
			switch(grid[y][x])
			{
				case Wall: 
					print(" * |");
					break;
				case Glenda:
					/* fancy effect for glenda's face */
					print(" %s |", turn % 2 ? gface[0] : gface[1]);
					break;
				default:
					print("   |");
					break;
			}
		}
		print("\n");
	}
	if(state == Won)
	{
		print("trapper won\n");
		exits(nil);
	}
	else if(state == Lost)
	{
		print("glenda won\n");
		exits(nil);
	}
}

/* p x y */
void
proc_put(char *s)
{
	unsigned int x, y, r;

	sscanf(s, "%u %n", &x, &r);
	sscanf(s+r, "%u", &y);
	
	if(x > SzX || x < 0 || y > SzY || y < 0)
	{
		fprint(2, "proc_put(): invalid input, x = %d, y = %d\n", x, y);
		return;
	}
	r = doput(Pt(x, y));
	if(r == Wall)
		fprint(2, "There is already a wall in x = %d, y = %d\n", x, y);
	else if(r == Glenda)
		fprint(2, "You can't put a wall on glenda!\n");
}

/* m x y */
void
proc_move(char *s)
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
	{
		fprint(2, "proc_move(): huh?\n");
		return;
	}
	
	/* should check if there is a wall or something this way */
	if(domove(d) == Wall)
	{
		fprint(2, "There is a wall there!\n");
		return;
	}
}

/*
 * handle input, which is in the form of
 * p1> p xx yy
 * which puts a wall in xx yy
 * p2> m {NE, E, SE, W, SW, NW}
 * which moves the bunny
 * > r
 * restarts the game
 * > u
 * undos last move
 * > q
 * quits the game
 */
void
proc(char *s)
{
	char *t;
	int oturn;

	/* skip \n, so we don't need to process it later */
	t = strchr(s, '\n');
	*t = '\0';

	oturn = turn;
	/* s+2 skips command and first space after it */
	switch(*s)
	{
		case 'p':
			if(turn % 2 == 0)
				proc_put(s+2);
			else if(turn % 2 == 1)
				fprint(2, "glendy can't put!\n");
			break;
		case 'm':
			if(turn % 2 == 0)
				fprint(2, "trapper can't move!\n");
			else if(turn % 2 == 1)
				proc_move(s+2);
			break;
		case 'u':
			undo();
			break;
		case 'r':
			/* maybe we need to put a confirm message here */
			restart();
			break;
		case 'q':
			exits(nil);
			break;
		case '\0':
			break;
		default:
			fprint(2, "proc(): huh?\n");
	}
	/* only print the map if turn have changed */
	if(turn != oturn)
		drawlevel();
}

int
input(void)
{	char *s, *r;
	int t;
	
	/* sang bozorg */
	s = malloc(1024);

	print("%s> ", turn % 2 ? "glendy" : "trapper");
	fflush(stdout); /* plan 9 */
	r = fgets(s, 1024, stdin);
	if(r == nil)
	{
		fprint(2, "input(): error\n");
		return Err;
	}
	else
	{
		proc(s);
		return Ok;
	}
}

int 
main(int argc, char **argv)
{
	char r;
	/*
	 * todo: handle argv properly
	 */
	ptype[0] = Human;
	ptype[1] = Human;
	/* OpenBSD ignores this */
	srand(time(nil));
	 
	initlevel();
	drawlevel();
	while(input() != Err)
		;

	return 0;
}
