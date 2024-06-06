#define playersock sockfd[turn % 2]
#define INPUTSIZE 512

extern int id;
extern int sockfd[2];

typedef struct
{
	int id;
	int difficulty;
	int state;
	int turn;
	int sockfd[2];
	int grid[SzX][SzY];
	char syncmsg[8];
}Game;

typedef struct
{
	int fd;
	char *nick;
	int game;	
	int side;
	int opts;
	void *thread;
}Client;
