/*
 * glendy (unix) server, 4th revision
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>

#include "unix.h"
#include "engine.h"
#include "util.h"
#include "srv.h"

#define playersock sockfd[turn % 2]

int listenfd;
int port = 1768;
int debug = 1;
char syncmsg[8];

int sockfd[2];

int gcount = 0;

int id = 0;

pthread_mutex_t game_lock;

Game games[32];
pthread_t threads[32][2];

static void
error(const char *msg)
{
	perror(msg);
	pthread_exit(NULL);
}

static void
cleanup(int game)
{
	dprint("cleanup(%d)\n", game);
	close(games[game].sockfd[0]);
	close(games[game].sockfd[1]);
	if(game > 0)
		gcount--;	
}

static void
printclients(char *fmt, ...)
{
	int i;
	/* it seems arg gets changed during the first call to vprint, thus we need two */
	va_list arg, arg2;
	
	va_start(arg, fmt);
	va_start(arg2, fmt);
	
	vfprint(sockfd[0], fmt, arg);
	vfprint(sockfd[1], fmt, arg2);
	va_end(arg);
	va_end(arg2);
}

int
setuplistener(int portno)
{
	int sockfd, option = 1;
	struct sockaddr_in serv_addr;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) 
		error("ERROR opening listener socket.");
	
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;	
	serv_addr.sin_addr.s_addr = INADDR_ANY;	
	serv_addr.sin_port = htons(portno);	
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
		error("ERROR binding listener socket.");

	dprint("listener on\n");

	return sockfd;
}

void
drawlevel(void)
{
	/* prints first row, this assumes SzX = SzY, is there a better way? */
	print("T%2d|", turn);
	for(int i = 0 ; i < SzX ; i++)
		print("%2d |", i);

	dprint(" G%2d\n", id);

	for(int x = 0; x < SzX; x++)
	{
		for(int i = 0 ; i < SzY+1 ; i++)
			print("----");
		print(x % 2 ? "\\" : "/");
		print("\n");

		/* show column number and have a zig-zag effect */
		print("%2d %s|", x, x % 2 ? "  " : "");

		for(int y = 0; y < SzY; y++)
		{
			/* it's [y][x], not [x][y] */
			switch(grid[y][x])
			{
				case Wall: 
					print(" * |");
					break;
				case Glenda:
					/* fancy effect for glenda's face */
					print(" g |");
					break;
				default:
					print("   |");
					break;
			}
		}
		print("\n");
	}
}

void
sendlevel(void)
{
	if(state == Start)
	{
		printclients("INIT\n");
	
		for(int x = 0; x < SzX; x++)
		{
			for(int y = 0; y < SzY; y++)
			{
				switch(grid[x][y])
				{
					case Wall: 
						printclients("w %d %d\n", x, y);
						break;
					case Glenda:
						printclients("g %d %d\n", x, y);
						break;
				}
			}
		}
		printclients("SENT\n");
	}
	else if(state == Playing)
	{
		printclients("SYNC %d %s\n", turn, syncmsg);
	}
	else
	{
		printclients("SYNC %d %s\n", turn, syncmsg);
		if(state == Won)
		{
			dprint("hah, trapper won\n");
			fprint(sockfd[0], "WON\n");
			fprint(sockfd[1], "LOST\n");
		}
		else if(state == Lost)
		{
			dprint("welp, trapper lost\n");
			fprint(sockfd[0], "LOST\n");
			fprint(sockfd[1], "WON\n");
		}
		cleanup(id);
	}
	dprint("TURN is %d\n", turn);
	fprint(playersock, "TURN\n");
	fprint(sockfd[!(turn % 2)], "WAIT\n");
}

/* p x y */
void
proc_put(char *s)
{
	char *xpos, *ypos;
	unsigned int x, y, r;

	xpos = strtok(s, " ");
	ypos = strtok(nil, " ");
	
	if(xpos == nil || ypos == nil)
	{
		fprint(playersock, "ERR INVALIDINPUT proc_put():"
		"not enough arguments or malformed string\n");
		return;
	}
	
	if(!isnum(xpos, strlen(xpos)) || !isnum(ypos, strlen(ypos)))
	{
		fprint(playersock, "ERR invalidinput proc_put():"
		"expected string in %s and %s\n", xpos, ypos);
		return;
	}
	
	x = atoi(xpos);
	y = atoi(ypos);
	
	dprint("put %d %d\n", x, y);

	if(x >= SzX || x < 0 || y >= SzY || y < 0)
	{
		fprint(playersock, "ERR invalidinput proc_put(): %d %d\n", x, y);
		return;
	}

	r = doput(Pt(x, y)); 
	if(r == Wall)
		fprint(playersock, "WALL %d %d\n", x, y);
	else if(r == Glenda)
		fprint(playersock, "GLND %d %d\n", x, y);
	else
	{
		sprint(syncmsg, "%u %u", x, y);
		/* better be safe than sorry */
		syncmsg[7] = '\0';
		dprint("syncmsg = %s\n", syncmsg);
	}
}

/* m x y */
void
proc_move(char *s)
{
	int d;
	Point p;
	
	p = findglenda();
	d = strtodir(s);
	
	if(d == Err)
	{
		fprint(playersock, "ERR invalidinput proc_move(): %s\n", s);
		return;
	}
	
	if(domove(d) == Wall)
	{
		fprint(playersock, "WALL %s %d %d\n", s, p.x, p.y);
		return;
	}
	else
	{
		strncpy(syncmsg, s, 7);
		/* better be safe than sorry */
		syncmsg[7] = '\0';
		dprint("syncmsg = %s\n", syncmsg);
	}
}

/*
 * handle input, which is in the form of:
 * trapper> p xx yy
 * puts a wall in xx yy
 * glenda> m {NE, E, SE, SW, W, NW}
 * moves the bunny
 * > q
 * quits the game
 */
int
proc(int player, char *s)
{
	char *t;
	int oturn, n;

	/* early return paths */
	if(*s == '\0' || *s == 'q')
	{
		/* should we end the game at this point? XXX important */
		fprint(sockfd[player], "DIE disconnected\n");
		fprint(sockfd[!player], "DIE other client have been disconnected\n");
		
		/* mmhm... what happens if we close a fd we are reading from? */
		cleanup(id);
		return Err;
	}
	else if(turn % 2 != player)
	{
		fprint(sockfd[player], "ERR WAIT\n");
		return Ok;
	}

	oturn = turn;
	/* s+2 skips command and first space after it */
	switch(*s)
	{
		case 'p':
			if(turn % 2 == 0)
				proc_put(s+2);
			else if(turn % 2 == 1)
				fprint(playersock, "CANTP\n");
			break;
		case 'm':
			if(turn % 2 == 0)
				fprint(playersock, "CANTM\n");
			else if(turn % 2 == 1)
				proc_move(s+2);
			break;
		default:
			fprint(playersock, "ERR proc() unkown command %c\n", *s);
	}
	/* only print the map if turn have changed */
	if(turn != oturn)
	{
		if(debug)
			drawlevel();
		sendlevel();
	}
	return Ok;
}

static char*
input(int game, int player)
{
	char *s, c;
	int n = 0;
	
	/* sang bozorg */
	s = emalloc(1024);
	memset(s, 0, 1024);
	
	/* we could use local variables, but that not worth the trouble */
	while((c = read(games[game].sockfd[player], s+n, 1) == 1) && n < 1024)
	{
		if(s[n] == '\n' || s[n] == '\0')
		{
			s[n] = '\0';
			break;
		}
		n++;
	}
	if(!strcmp(s, ""))
		dprint("got input: %s\n", s);

	return s;
}

/*
 * most of game engine's works on global variables
 * set the global variables with one game from the array
 */
static void
loadgame(int n)
{
	if(n > sizeof(games) / sizeof(Game))
		sysfatal("loadgame(%d): invalid game", n);
	
	id = games[n].id;
	difficulty = games[n].difficulty;
	state = games[n].state;
	turn = games[n].turn;

	strncpy(syncmsg, games[n].syncmsg, 8);
	
	sockfd[0] = games[n].sockfd[0];
	sockfd[1] = games[n].sockfd[1];
	
	memcpy(grid, games[n].grid, sizeof(grid));
}

static void
setgame(int n)
{
	if(n > sizeof(games) / sizeof(Game))
		sysfatal("setgame(%d): invalid game", n);
	
	/* id got to change when we set game */
	id = n;
	games[n].id = n;
	games[n].difficulty = difficulty;
	games[n].state = state;
	games[n].turn = turn;

	games[n].sockfd[0] = sockfd[0];
	games[n].sockfd[1] = sockfd[1];

	memcpy(games[n].grid, grid, sizeof(grid));
}

/* player is either 0 or 1, trapper or glenda */
static void
clienthandler(void *data)
{
	int res, player, game;
	char *s;
	
	game = ((int*)data)[0];
	player = ((int*)data)[1];
	
	for(;;)
	{
		/* most of time is spent here */
		s = input(game, player);
		

		if(!strcmp(s, ""))
		{
			cleanup(game);
			break;
		}
		
		pthread_mutex_lock(&game_lock);
		
		loadgame(game);
		res = proc(player, s);
		setgame(game);
		
		pthread_mutex_unlock(&game_lock);
		free(s);
	}
}

void
srv(int listenfd)
{
	int clientfd[2], res[2];
	socklen_t clilen;
	struct sockaddr_in servaddr, clientaddr;
	int tdata[2][2];

	while(gcount < 32)
	{
		for(int conns = 0 ; conns < 2 ; conns++)
		{
			listen(listenfd, 64);
			memset(&clientaddr, 0, sizeof(clientaddr));
	
			clilen = sizeof(clientaddr);
			clientfd[conns] = accept(listenfd, (struct sockaddr *) &clientaddr, &clilen);
		
			if(clientfd[conns] < 0)
				error("srv(): failed to accept connection");
	
			fprint(clientfd[conns], "CONN %d\n", conns);
			dprint("srv(): client %d connected\n", conns);
	
			if(conns == 0)
				fprint(clientfd[conns], "WAIT\n");
		}
		pthread_mutex_lock(&game_lock);
		gcount++;

		sockfd[0] = clientfd[0];
		sockfd[1] = clientfd[1];

		initlevel();
		setgame(gcount);

		if(debug)
			drawlevel();
		
		sendlevel();

		pthread_mutex_unlock(&game_lock);


		tdata[0][0] = gcount;
		tdata[0][1] = 0;
		
		tdata[1][0] = gcount;
		tdata[1][1] = 1;
		
		res[0] = pthread_create(&threads[gcount][0], NULL, (void*)clienthandler, (void*)tdata[0]);
		res[1] = pthread_create(&threads[gcount][1], NULL, (void*)clienthandler, (void*)tdata[1]);
		
		if(res[0] || res[1])
		{
			dprint("pthread_create() failed: %d\n", res[0] ? res[0] : res[1]);
			exit(-1);
		}
		else
			dprint("threads for game %d are created\n", gcount);
	}
}

int 
main(int argc, char **argv)
{
	/* it might not be a real human */
	ptype[0] = Human;
	ptype[1] = Human;
	
	listenfd = setuplistener(port);
	pthread_mutex_init(&game_lock, NULL);
	
	/* OpenBSD ignores this */
	srand(time(nil));
	
	srv(listenfd);
	
	close(listenfd);
	pthread_mutex_destroy(&game_lock);
	pthread_exit(NULL);
	return 0;
}
