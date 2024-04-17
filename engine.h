enum
{
	/* difficulty levels (how many circles are initially occupied) */
	DEasy,	/* 10≤x<15 */
	DMed,	/* 5≤x<10 */
	DHard,	/* 1≤x<5 */
	DImp, /* 0 */

	New = 0,
	Undo,
	Restart,
	Exit,

	/* dynamic? original game has a fixed grid size, but we don't need to abide by it */
	SzX = 11,
	SzY = 11, 

	Border = 3,

	/* movement directions */
	NE,
	E,
	SE,
	SW,
	W,
	NW,

	/* player types */
	Human,
	Computer,
	
	Start = 0, /* game states */
	Playing,
	Won,	
	Lost,

	Prev = 100,
	Wall = 999,
	Glenda = 1000,

	Ok,
	Err,
};

extern int difficulty;
extern int state;
extern int turn;
extern int ptype[2];

extern int grid[SzX][SzY];
extern int pgrid[SzX][SzY]; /* for undo */
extern int ogrid[SzX][SzY]; /* so we can restart levels */

void initlevel(void);
Point movedir(int dir, Point p);
int pointdir(Point src, Point dst);
int domove(int dir, Point p);
int doput(Point p);
Point findglenda(void);
int checknext(int dir, Point p);
int score1(Point p);
void calc(void);
void nextglenda(void);
void restart(void);
void undo(void);
int checkstate(void);
