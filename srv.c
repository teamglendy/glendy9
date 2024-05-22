/*
 * glendy (unix) server, based on cli version.
 *
 * it appears to my mind that socket.h and stdio.h have are in use
 * since medieval times, which is really impressive of a design;
 * ...for a torture device.
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
// #include "srv.h"

#define playersock sockfd[turn % 2]

int listenfd;
int port = 1768;
int pcount = 0;
int sockfd[2];
int debug = 1;
char syncmsg[8];

pthread_mutex_t pcount_mutex;
// pthread_mutex_t print_mutex;

static void
error(const char *msg)
{
	perror(msg);
	pthread_exit(NULL);
}

static void
printclients(char *fmt, ...)
{
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
getclients(int lis_sockfd)
{
	int conns = 0;
	socklen_t clilen;
	struct sockaddr_in servaddr, clientaddr;
	
	while(conns < 2)
	{
		listen(lis_sockfd, 253 - pcount);
		memset(&clientaddr, 0, sizeof(clientaddr));

		clilen = sizeof(clientaddr);
		sockfd[conns] = accept(lis_sockfd, (struct sockaddr *) &clientaddr, &clilen);
	
		if(sockfd[conns] < 0)
			error("ERROR accepting a connection from a client.");

		dprint("got connection %d\n", conns);
		fprint(sockfd[conns], "CONN %d\n", conns);
		dprint("client %d connected\n", conns);
		
		pthread_mutex_lock(&pcount_mutex);
		pcount++;
		dprint("pcount = %d\n", pcount);
		pthread_mutex_unlock(&pcount_mutex);

		if(conns == 0)
		{
			fprint(sockfd[conns], "WAIT\n");
			dprint("someone is gonna wait until they friend comes\n");
		}

		conns++;
	}
}


void
drawlevel(void)
{
	/* prints first row, this assumes SzX = SzY, is there a better way? */
	print("T%2d|", turn);
	for(int i = 0 ; i < SzX ; i++)
		print("%2d |", i);

	print("\n");

	for(int x = 0; x < SzX; x++)
	{
		for(int i = 0 ; i < SzY+1 ; i++)
			print("----");
		print(x % 2 ? "\\" : "/");
		print("\n");

		/* show column number and have a zig-zag effect */
		print("%2d%s |", x, x % 2 ? "  " : "");

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
				switch(grid[y][x])
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
			close(sockfd[0]);
			close(sockfd[1]);
		}
	}
	fprint(playersock, "TURN\n");
	fprint(sockfd[!(turn % 2)], "WAIT\n");
}

/* p x y */
void
proc_put(char *s)
{
	char *xpos, *ypos;
	unsigned int x, y, r;

	ypos = strtok(s, " ");
	xpos = strtok(nil, " ");
	
	if(xpos == nil || ypos == nil)
	{
		fprint(playersock, "ERR invalidinput proc_put():"
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

	r = doput(Pt(y, x)); 
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
 * glenda> m {NE, E, SE, W, SW, NW}
 * moves the bunny
 * > q
 * quits the game
 */
int
proc(char *s, int player)
{
	char *t;
	int oturn, n;

	/* early return paths */
	if(*s == '\0' || *s == 'q')
	{
		/* should we end the game at this point? XXX important */
		fprint(sockfd[player], "DIE disconnected\n");
		fprint(sockfd[!player], "DIE other client have been disconnected\n");
		
		/* mmhm... one may wonder what happens if
		 * we close a fd that happens to be read from? */
		close(sockfd[0]);
		close(sockfd[1]);
		pcount -= 2;
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
input(int player)
{
	char *s, c;
	int n = 0;
	
	/* sang bozorg */
	s = malloc(1024);
	
	memset(s, 0, 1024);
	while(read(sockfd[player], s+n, 1) == 1 && n < 1024)
	{
		if(s[n] == '\n' || s[n] == '\0')
		{
			s[n] = '\0';
			break;
		}
		n++;
	}
	dprint("got input: %s\n", s);

	return s;
}

/* player is either 0 or 1, trapper or glenda */
static void
clienthandler(void *player)
{
	char *s;
	int p;
	p = *(int*)player;
	
	for(;;)
	{
		s = input(p);
	
		proc(s, p);
		free(s);
	}
}

static void
srv(void)
{
	int res[2];
	/* meh. i can't have pointers like &(1) in ANSI */
	int zero = 0, one = 1;
	pthread_t p1_thread, p2_thread;
	for(;;)
	{
		getclients(listenfd);
		initlevel();

		if(debug)
			drawlevel();
		
		sendlevel();
		
		res[1] = pthread_create(&p2_thread, NULL, (void*)clienthandler, &one);
		res[0] = pthread_create(&p1_thread, NULL, (void*)clienthandler, &zero);
		if(res[0] || res[1]){
			dprint("pthread_create() failed: %d\n", res[0] ? res[0] : res[1]);
			exit(-1);
		}	
	
	}
}

int 
main(int argc, char **argv)
{
	/* it might not be a real human */
	ptype[0] = Human;
	ptype[1] = Human;
	
	listenfd = setuplistener(port);
	pthread_mutex_init(&pcount_mutex, NULL);
	
	/* OpenBSD ignores this */
	srand(time(nil));
	
	srv();
	
	close(listenfd);
	pthread_mutex_destroy(&pcount_mutex);
	pthread_exit(NULL);
	return 0;
}
