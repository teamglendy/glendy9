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

char *gface[2] = {"g", "g"}; /* glenda's face(es) */

void
drawlevel(void)
{
	int x, y;
	
	/* prints first row, this assumes SzX = SzY, is there a better way? */
	printf("T%2d|", turn);
	for(int i = 0 ; i < SzX ; i++)
		printf("%2d |", i);

	printf("\n");

	for(x = 0; x < SzX; x++)
	{
		for(int i = 0 ; i < SzY+1 ; i++)
			printf("----");
		printf("\n");

		/* show column number and have a zig-zag effect */
		printf("%2d%s |", x, x % 2 ? "" : "| ");
		// printf("%2d |", x);

		for(y = 0; y < SzY; y++)
		{
			/* it's [y][x], not [x][y] */
			switch(grid[y][x])
			{
				case Wall: 
					printf(" * |");
					break;
				case Glenda:
					/* fancy effect for glenda's face */
					printf(" %s |", turn % 2 ? gface[0] : gface[1]);
					break;
				default:
					printf("   |");
					break;
			}
		}
		printf("\n");
	}
	if(state == Won)
	{
		printf("trapper won\n");
		exits(nil);
	}
	else if(state == Lost)
	{
		printf("glenda won\n");
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
		fprintf(stderr, "proc_put(): invalid input, x = %d, y = %d\n", x, y);
		return;
	}

	if(doput(Pt(x, y)) == Wall)
		fprintf(stderr, "There is already a wall in x = %d, y = %d\n", x, y);
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
		fprintf(stderr, "proc_move(): huh?\n");
		return;
	}
	
	/* should check if there is a wall or something this way */
	if(domove(d, findglenda()) == Wall)
	{
		fprintf(stderr, "There is a wall there!\n");
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
				fprintf(stderr, "glendy can't put!\n");
			break;
		case 'm':
			if(turn % 2 == 0)
				fprintf(stderr, "trapper can't move!\n");
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
		default:
			fprintf(stderr, "proc(): huh?\n");
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

	printf("%s> ", turn % 2 ? "glendy" : "trapper");
	fflush(stdout); /* plan 9 */
	r = fgets(s, 1024, stdin);
	if(r == nil)
	{
		fprintf(stderr, "input(): error\n");
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

	/* OpenBSD ignores this
	 *	srand(time(nil));
	 */
	initlevel();
	drawlevel();
	while(input() != Err);

	return 0;
}
