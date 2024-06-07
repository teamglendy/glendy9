#define playersock sockfd[turn % 2]
#define INPUTSIZE 32

extern int id;
extern int sockfd[2];

typedef struct
{
	int fd;
	char *nick;
	int game;	
	int side;
	int opts;
	int state;
	long firstseen;
	long lastseen;
	
	void *thread;
}Client;

typedef struct
{
	int id;
	int difficulty;
	int state;
	int turn;
	int sockfd[2];
	int grid[SzX][SzY];
	char syncmsg[8];
	Client *clients[2];
}Game;
