typedef struct
{
	int fd;
	char[16] cookie;

}Client;

typedef struct
{
	int id;
	List msgs;
	int state;
	int turn;
	Client player[2];
	int grid[SzX][SzY];
}Game;
