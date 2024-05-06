#ifdef unix
#include "unix.h"
#else
#include <u.h>
#include <libc.h>
#include <draw.h>
#endif

#include "engine.h"

int difficulty = DEasy;
int state;
int turn = 0;
int ptype[2] = {Human, Computer}; /* human? computer? */

int grid[SzX][SzY];
int pgrid[SzX][SzY]; /* for undo */
int ogrid[SzX][SzY]; /* so we can restart levels */

void
initlevel(void)
{
	int i, cnt, x, y;

	for(x = 0; x < SzX; x++)
		for(y = 0; y < SzY; y++)
			ogrid[x][y] = Prev;

	switch(difficulty)
	{
		case DEasy:
			cnt = 10 + nrand(5);
			break;
		case DMed:
			cnt = 5 + nrand(5);
			break;
		case DHard:
			cnt = 1 + nrand(5);
			break;
		case DImp:
		default:
			cnt = 0;
			break;
	}
	ogrid[SzX/2][SzY/2] = Glenda;

	for(i = 0 ; i < cnt ; i++)
	{
		/*
		it's unlikely, but possible:
		we randomly pick a cell which already have a wall or glenda inside it
		*/
		do
		{
			x = nrand(SzX);
			y = nrand(SzY);
		}while(ogrid[x][y] != Prev);
		ogrid[x][y] = Wall;
	}

	memcpy(grid, ogrid, sizeof grid);
	state = Start;
}

Point
movedir(int dir, Point p)
{
	int x = p.x;
	int y = p.y;
	
	switch(dir)
	{
		case NE: 
			return Pt(x+(y%2?1:0), y-1);
		case E:
			return Pt(x+1, y);
		case SE:
			return Pt(x+(y%2?1:0), y+1);
		case SW:
			return Pt(x+(y%2?0:-1), y+1);
		case W:
			return Pt(x-1, y);
		case NW:
			return Pt(x+(y%2?0:-1), y-1);
		default:
			sysfatal("andrey messed up big time");
			/* should we keep that line around? it might be more useful than sysfatal */
			// return Pt(-1, -1); 
	}
}

/* reverse of movedir, tells the direction for a dst from src */
int
pointdir(Point src, Point dst)
{
	Point p;
	/*
	if(src.x < 0 || src.x > SzX || src.y < 0 || src.y > SzY)
	||(dst.x < 0 || dst.x > SzX || dst.y < 0 || dst.y > SzY)
		return Err;
	*/
	
	/* hacky */
	for(int i = NE ; i <= NW ; i++)
	{
		p = movedir(i, src);
		if(p.x == dst.x && p.y == dst.y)
			return i;
	}
	return Err;
}

/*
Point
movedir(int dir, Point p)
{
	switch(dir)
	{
		case NE:
			if(p.y % 2)
				return Pt(p.x+1, p.y-1);
			else	
				return Pt(p.x, p.y-1);
		case E:
			return Pt(p.x+1, p.y);
		case SE:
			if(p.y % 2)
				return Pt(p.x+1, p.y+1);
			else
				return Pt(p.x, p.y+1);
		case SW:
			if(p.y % 2)
				return Pt(p.x, p.y+1);
			else
				return Pt(p.x-1, p.y+1);
		case W:
			return Pt(p.x-1, p.y);
		case NW:
			if(p.y % 2)
				return Pt(p.x, p.y-1);
			else
				return Pt(p.x-1, p.y-1);
		default:
			return Pt(-1, -1);
	}
}
*/

int
domove(int dir)
{
	Point src, dst;
	
	src = findglenda();
	if(src.x == 0 || src.x == SzX-1 || src.y == 0 || src.y == SzY-1)
		goto done;

	dst = movedir(dir, src);
	if(grid[dst.x][dst.y] == Wall)
		return Wall;

	grid[dst.x][dst.y] = Glenda;
done:
	grid[src.x][src.y] = Prev;

	turn++;
	return 0;
}

int
doput(Point p)
{
	/* clients are expected to do their own error checking */
	if(p.x > SzX || p.x < 0 || p.y > SzY || p.y < 0)
		return Err;

	if(grid[p.x][p.y] == Wall)
		return Wall;

	if(grid[p.x][p.y] == Glenda)
		return Glenda;

	/* take a copy for undo */
	memcpy(pgrid, grid, sizeof grid);
	grid[p.x][p.y] = Wall;


	/* assumes defenders start game first */
	if(state == Start)
		state = Playing;

	turn++;
	/* reset the board scores */
	for(int x = 0; x < SzX; x++)
		for(int y = 0; y < SzY; y++)
			if(grid[x][y] != Wall && grid[x][y] != Glenda)
				grid[x][y] = 100;

	/* we need it to check game state, even if not playing with computer */
	nextglenda();
	return Ok;

}

Point
findglenda(void)
{
	for(int x = 0; x < SzX; x++)
		for(int y = 0; y < SzY; y++)
			if(grid[x][y] == 1000)
				return Pt(x, y);
	
	return Pt(-1, -1);
}

int
checknext(int dir, Point p)
{
	int x = p.x;
	int y = p.y;
	
	switch(dir)
	{
		case NE: 
			return grid[x+(y%2?1:0)][y-1];
		case E:
			return grid[x+1][y];
		case SE:
			return grid[x+(y%2?1:0)][y+1];
		case SW:
			return grid[x+(y%2?0:-1)][y+1];
		case W:
			return grid[x-1][y];
		case NW:
			return grid[x+(y%2?0:-1)][y-1];
		default:
			sysfatal("andrey messed up big time");
			// return -1; /* silence compiler */
	}
}

/* the following two routines constitute the "game AI"
* they score the field based on the number of moves
* required to reach the edge from a particular point
* scores > 100 are "dead spots" (this assumes the field 
* is not larger than ~100*2
* 
* routines need to run at least twice to ensure a field is properly
* scored: there are errors that creep up due to the nature of 
* traversing the board
*/
int 
score1(Point p)
{
	
	int min = 999, next;

	if(p.x == 0 || p.x == SzX-1 || p.y == 0 || p.y == SzY-1)
		return 1; 		/* we can always escape from the edges */

	for(int dir = NE; dir <= NW; dir++)
	{
		next = checknext(dir, p);
		if(next < min)
			min = next;
	}
	if(min == 999)
		return 998;
	return 1+min;
}

void
calc(void)
{
	for(int i = 0; i < SzX; i++) /* assumes SzX = SzY */
		for(int x = i; x < SzX-i; x++)
			for(int y = i; y < SzY-i; y++)
				if(grid[x][y] != Wall && grid[x][y] != Glenda)
					grid[x][y] = score1(Pt(x, y));
}

void
nextglenda(void)
{
	int min = 1000, next, dir, nextdir = 0, count = 0;
	Point p = findglenda();

	calc();
	calc();
	calc();
	
	for(dir = NE; dir <= NW; dir++)
	{
		next = checknext(dir, p);
		if(next < min)
		{
			min = next;
			nextdir = dir;
			++count;
		}
		else if(next == min)
			nextdir = (nrand(++count) == 0) ? dir : nextdir;
	}
	if(min > 100 && min != 999)
			state = Won;	
	else if(ptype[1] == Computer)
		domove(nextdir);

	if(eqpt(findglenda(), Pt(-1, -1)))
		state = Lost;
}

void
restart(void)
{
	turn = 0;
	memcpy(grid, ogrid, sizeof grid);
	state = Start;
}

void
undo(void)
{
	int g[SzX][SzY];

	if(state == Start || turn < 2)
		return;

	/* revert turn counter, can be abused by one side */
	turn -= 2;

	/* swap grids */
	memcpy(g, grid, sizeof grid);
	memcpy(grid, pgrid, sizeof grid);
	memcpy(pgrid, g, sizeof grid);
}

int
checkstate(void)
{
	int i, j;
	for(i = 0; i < SzX; i++)
		for(j = 0; j < SzY; j++)
			if(grid[i][j] == 'E')
				return 0;
	return 1;
}
