/*
 * glendy (unix) server, 5th revision
 * leaks memory, often, quick and well.
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
#include <ctype.h>

#include "unix.h"
#include "engine.h"
#include "util.h"
#include "srv.h"

int listenfd;
int port = 1768;
int debug = 1;

char syncmsg[8];
int sockfd[2];
int gcount = 0;
int ccount = 0;
int id = 0;

pthread_mutex_t game_lock;

List *games;
List *threads;
Quene clients;

static void
error(const char *msg)
{
	perror(msg);
	pthread_exit(NULL);
}

static void
cleanup(int gid)
{
	Game *g;
	
	dprint("cleanup(%d)\n", gid);
	
	g = (Game*)lookup(games, gid);
	shutdown(g->sockfd[0], SHUT_RDWR);
	shutdown(g->sockfd[1], SHUT_RDWR);
	close(g->sockfd[0]);
	close(g->sockfd[1]);
	
	if(g->state != Finished)
		g->state = Finished;
	else
	/* we can't delete whole list because we need to update gid for rest of games too! */
		free(g);
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
		printclients("SYNC %d %s\n", turn, syncmsg);
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
		return;
	}
	dprint("Game %d, Turn is %d, min = %d\n", id, turn, findmin(findglenda()));
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

	/* engine assumes it's XY, protocol assumes it's YX */
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
		fprint(playersock, "ERR wall in %d %d\n", x, y);
	
	else if(r == Glenda)
		fprint(playersock, "ERR glenda in %d %d\n", x, y);
	
	else
	{
		sprint(syncmsg, "%u %u", x, y);
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
		fprint(playersock, "ERR invalidinput proc_move(): %s\n", s);
	
	else if(domove(d) == Wall)
		fprint(playersock, "ERR glenda %s %d %d\n", s, p.x, p.y);
	
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
	int oturn;

	/* early return paths */
	if(*s == '\0' || *s == 'q')
	{
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
				fprint(playersock, "ERR you can't put walls\n");
			break;
		case 'm':
			if(turn % 2 == 0)
				fprint(playersock, "you can't move!\n");
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
input(int gid, int player)
{
	char *s, c;
	int n = 0;
	Game *g;
	
	/* sang bozorg */
	s = (char*)emalloc(INPUTSIZE);
	memset(s, 0, INPUTSIZE);
	
	g = (Game*)lookup(games, gid);
	
	/* we could use local variables, but that not worth the trouble */
	while((c = read(g->sockfd[player], s+n, 1) == 1) && n < INPUTSIZE)
	{
		/* silently drop CR from CRLF */
		if(s[n] == '\r')
			s[n] = '\0';	
		if(s[n] == '\n' || s[n] == '\0')
		{
			s[n] = '\0';
			break;
		}
		n++;
	}
	if(strcmp(s, ""))
		dprint("input(%d, %d): got input: 0x%x, %s\n", gid, player, *s, s);

	return s;
}

/*
 * most of game engine's works on global variables
 * set the global variables with one game from the array
 */
static void
loadgame(int gid)
{
	Game *g;
//	if(n > sizeof(games) / sizeof(Game))
//		sysfatal("loadgame(%d): invalid game", n);
	
	dprint("loadgame(%d)\n", gid);
	g = (Game*)lookup(games, gid);
	
	id = g->id;
	difficulty = g->difficulty;
	state = g->state;
	turn = g->turn;

	strncpy(syncmsg, g->syncmsg, 8);
	
	sockfd[0] = g->sockfd[0];
	sockfd[1] = g->sockfd[1];
	
	memcpy(grid, g->grid, sizeof(grid));
}

static void
setgame(int gid)
{
	Game *g;
//	if(n > lllen(games))
//		sysfatal("setgame(%d): invalid game", n);
	
	dprint("loadgame(%d)\n", gid);
	g = (Game*)lookup(games, gid);
	
	/* id got to change when we set game */
	id = gid;
	g->id = gid;
	g->difficulty = difficulty;
	g->state = state;
	g->turn = turn;

	g->sockfd[0] = sockfd[0];
	g->sockfd[1] = sockfd[1];

	memcpy(g->grid, grid, sizeof(grid));
}

/* player is either 0 or 1, trapper or glenda */
static void
clienthandler(void *data)
{
	int gid, player;
	char *s;
	
	gid = ((int*)data)[0];
	player = ((int*)data)[1];
	
	dprint("clienthandler({%d, %d})\n", gid, player);
		
	for(;;)
	{
		/* most of time is spent here */
		s = input(gid, player);
		
		if(!strcmp(s, ""))
			break;
		
		pthread_mutex_lock(&game_lock);
		
		loadgame(gid);
		proc(player, s);
		setgame(gid);
		
		pthread_mutex_unlock(&game_lock);
		free(s);
	}
	cleanup(gid);
	free(data);
}
static void
play(Client *c1, Client *c2)
{
	int res[2];
	int *tdata[2];
	Game *g;
	
	tdata[0] = (int*)emalloc(2 * sizeof(int));
	tdata[1] = (int*)emalloc(2 * sizeof(int));
	
	pthread_mutex_lock(&game_lock);
	gcount++;
	
	sockfd[0] = c1->fd;
	sockfd[1] = c2->fd;

	g = (Game*)emalloc(sizeof(Game));
	llappend(games, g);
	initlevel();
	setgame(gcount);

	if(debug)
		drawlevel();
	
	fprint(g->sockfd[0], "CONN %d %s\n", 0, c2->nick);
	fprint(g->sockfd[1], "CONN %d %s\n", 1, c1->nick);
	
	sendlevel();

	tdata[0][0] = gcount;
	tdata[0][1] = 0;
	
	tdata[1][0] = gcount;
	tdata[1][1] = 1;
	
	c1->thread = (pthread_t*)emalloc(sizeof(pthread_t));
	c2->thread = (pthread_t*)emalloc(sizeof(pthread_t));
	
//	llappend(threads, c1->thread);
//	llappend(threads, c2->thread);
	
	pthread_mutex_unlock(&game_lock);
	
	res[0] = pthread_create(c1->thread, nil, (void*)clienthandler, (void*)tdata[0]);
	res[1] = pthread_create(c2->thread, nil, (void*)clienthandler, (void*)tdata[1]);
	
	dprint("play(): tdata[0] {%d, %d}\n", tdata[0][0], tdata[0][1]);
	dprint("play(): tdata[1] {%d, %d}\n", tdata[1][0], tdata[1][1]);
	
	if(res[0] || res[1])
		sysfatal("pthread_create() failed: %d\n", res[0] ? res[0] : res[1]);

	dprint("play(): threads for game %d are created\n", gcount);	
}

char*
parsenick(int fd, char *nick)
{
	char *s;
	int len = strlen(nick);
	
	if(len > 8)
	{
		fprint(fd, "DIE nick too long: %d\n", len);
		goto die;
	}
	
	for(int i = 0 ; len > i ; i++)
	{
		if(!isalnum(nick[i]))
		{
			fprint(fd, "DIE nicks must be only alpha numeric strings\n");
			goto die;
		}
	}
	
	s = strdup(nick);
	return s;
	
	die:
			close(fd);
			return nil;
}

int
parsegame(int fd, char *game)
{
	int g;
	
	g = atoi(game);
	if(g == 0)
		return g;
	
	fprint(fd, "DIE invalid game: %d", g);
	close(fd);
	return -1;
}


int
parseside(int fd, char *side)
{
	int s;
	
	s = atoi(side);
	if(s == PGlenda || s == PTrapper || s == PRandom)
		return s;
	
	fprint(fd, "DIE invalid side: %d", s);
	close(fd);
	return -1;
}

int
parseopts(int fd, char *opts)
{
	return atoi(opts);
}

Client*
newclient(char *in, int fd)
{
	Client *c;
	char *nick, *side, *game, *opts;

	nick = strtok(in, " ");
	game = strtok(nil, " ");
	side = strtok(nil, " ");
	opts = strtok(nil, " ");

	dprint("newclient(%d): nick: %s, side: %s, game: %s, opts: %s\n", fd, nick, side, game, opts);
	
	if(nick == nil || side == nil || game == nil)
			return nil;
	
	c = (Client*)emalloc(sizeof(Client));
	c->fd = fd;
	c->nick = parsenick(fd, nick);
	c->game = parsegame(fd, game);	
	c->side = parseside(fd, side);
	c->opts = parseopts(fd, opts);
	
	if(c->nick == nil || c->game == -1 || c->side == -1)
	{
		free(c);
		close(fd);
		return nil;
	}
	
	ccount++;
	return c;
}

void
makematch(Client *c)
{
	Client *head;

	dprint("makematch(%p)\n", c);
	pthread_mutex_lock(&game_lock);
	if(clients.head == nil)
	{
		clients.head = llnew();
		clients.tail = clients.head;

	}
	head = (Client*)clients.head->data;
	if(clients.len == 0 || c->side == head->side)
	{
		/* head->side can never be PRandom anyway */
		if(c->side == PRandom)
			c->side = nrand(1) ? PTrapper : PGlenda;
		
		qadd(&clients, c);
		pthread_mutex_unlock(&game_lock);
		fprint(c->fd, "WAIT\n");
	}
	else
	{
		if(c->side == PRandom)
			c->side = (head->side == PGlenda) ? PTrapper : PGlenda;
		
		qnext(&clients);
		pthread_mutex_unlock(&game_lock);
		if(c->side == PTrapper)
			play(c, head);
		else
			play(head, c);
	}
}

void
registerclient(void *clientfd)
{
	char c, *s;
	Client *cl;
	int n = 0;
	int fd = *(int*)clientfd;
	
	dprint("registerclient(%d)\n", fd);
	
	/* sang bozorg */
	s = (char*)emalloc(INPUTSIZE);
	memset(s, 0, INPUTSIZE);
	

	/* we could use local variables, but that not worth the trouble */
	while((c = read(fd, s+n, 1) == 1) && n < INPUTSIZE)
	{
		/* silently drop CR from CRLF */
		if(s[n] == '\r')
			s[n] = '\0';
		else if(s[n] == '\n' || s[n] == '\0')
		{
			s[n] = '\0';
			break;
		}
		n++;
	}
	
	if(!strcmp(s, ""))
	{
		dprint("registerclient(%d): got empty string\n", fd);
		fprint(fd, "DIE empty string\n");
		close(fd);
		goto die;
	}
	
	dprint("registerclient(%d): got input: %x, %s\n", fd, *s, s);
	cl = newclient(s, fd);
	
	if(cl != nil)
		makematch(cl);
	die:
		free(s);
}

void
srv(int listenfd)
{
	int *clientfd;
	socklen_t clilen;
	struct sockaddr_in clientaddr;
	pthread_t t;
	
	for(;;)
	{
		listen(listenfd, 64);
		memset(&clientaddr, 0, sizeof(clientaddr));
		
		clilen = sizeof(clientaddr);
		clientfd = (int*)emalloc(sizeof(int));
		*clientfd = accept(listenfd, (struct sockaddr *) &clientaddr, &clilen);
		
		if(*clientfd < 0)
			error("srv(): failed to accept connection");
		
		dprint("game %d: srv(): client %d connected, fd: %d\n", gcount, ccount, *clientfd);
		pthread_create(&t, NULL, (void*)registerclient, (void*)clientfd);
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
	
	games = llnew();
	clients.len = 0;
	clients.head = llnew();
	clients.tail = clients.head;
	threads = llnew();
	srv(listenfd);
	
	close(listenfd);
	pthread_mutex_destroy(&game_lock);
	pthread_exit(NULL);
	return 0;
}
