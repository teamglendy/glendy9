#define isplaying (state == Playing || state == Start)

enum
{
	/* difficulty levels (how many circles are initially occupied) */
	DEasy,	/* 10≤x<15 */
	DMed,	/* 5≤x<10 */
	DHard,	/* 1≤x<5 */
	DImp, /* 0 */

	/* dynamic? original game has a fixed grid size, but we don't need to abide by it */
	SzX = 11,
	SzY = 11,

	/* movement directions */
	NE,
	E,
	SE,
	SW,
	W,
	NW,

	/* player types */
	Human = 0,
	Computer,
	Net,

	PTrapper = 0,	
	PGlenda,
	PRandom,

	/* game states */
	Connect = 0,
	Init, /* setting up the map */
	Start,
	Playing,
	Won,	
	Lost,
	Finished, /* for server */

	Prev = 100,
	Wall = 999,
	Glenda = 1000,

	Err = 0,
	Ok,
};

/* engine.c */
extern int difficulty;
extern int state;
extern int turn;
extern int ptype[2]; /* Human or Computer? */

extern int grid[SzX][SzY];
extern int pgrid[SzX][SzY]; /* for undo */
extern int ogrid[SzX][SzY]; /* so we can restart levels */

/* client code */
extern int debug;

/* net.c */
/* we maybe be able to merge all this into one bit-array */
extern int waitbit; /* 0 is go, 1 is wait */
extern int networked; /* 0 is local, 1 is networked */
extern int pside; /* Trapper, Glenda */

void initlevel(void);
Point movedir(int dir, Point p);
int pointdir(Point src, Point dst);

int domove(int dir);
int doput(Point p);

Point findglenda(void);
int checknext(int dir, Point p);

int score1(Point p);
void calc(void);
void nextglenda(void);

int findmin(Point p);
int checkstate(void);

void restart(void);
void undo(void);
