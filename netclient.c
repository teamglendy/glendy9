#ifdef unix
#include "unix.h"
#else
#include <u.h>
#include <libc.h>
#include <draw.h>
#endif
#include "engine.h"
#include "netclient.h"

int waitbit; /* 0 is go, 1 is wait */
int networked; /* 0 is local, 1 is networked */
int pside; /* Trapper, Glenda */

int sockfd;


static int
dprint(char *fmt, ...)
{
	va_list va;
	int n;

	if(!debug)
		return 0;
	
	va_start(va, fmt);
	n = vfprint(2, fmt, va);
	va_end(va);
	return n;
}

static char*
dirtostr(int dir)
{
	switch(dir)
	{
		case NE:
			return "NE";
		case E:
			return "E";
		case SE:
			return "SE";
		case SW:
			return "SW";
		case W:
			return "W";
		case NW:
			return "NW";
		default:
			return nil;
	}
}

/* returned value might be nil */
static char*
getpart(char *s, char *tok, unsigned int n)
{
	char *tmp = nil;
	
	for(int i = 0 ; i < n ; i++)
		tmp = strtok(s, tok);
	
	return tmp;
}

static int
isnum(char *s, unsigned int n)
{
	if(strlen(s) < n)
		n = strlen(s);
	
	for(int i = 0 ; i < n ; i++)
	{
		if(s[i] > '9' || s[i] < '0')
			return 0;
	}
	return 1;
}

/* TODO: move to engine.h */
static int
parsemove(char *s)
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
		sysfatal("parsemove(): invalid direction");

	return d;
}

static char*
movemsg(int dir)
{
	char *d, *msg;

	d = dirtostr(dir);
	if(d != nil)
	{
		msg = malloc(8);
		sprint(msg, "m %s\n", d);
		return msg;
	}
	else
		return nil;
}

/* xx yy\0 */
/*
static Point
parseput(char *s)
{
	int x, y;
	int len;
	
	x = atoi(s);
	s = strchr(s, ' ');
	if(end == nil)
	{
		dprint("parseput(): end nil\n");
		sysfatal("parseput(): incomplete line");
	}
	y = atoi(s);
	return Pt(x, y);
}
*/

static Point
parseput(char *x, char *y)
{
	if(isnum(x, 2) != 1 && isnum(y, 2) != 1)
		sysfatal("parseput(): input isnt a number?");
	
	return Pt(atoi(x), atoi(y));
}

static char*
putmsg(int x, int y)
{
	char *msg;

	msg = malloc(10);
	sprint(msg, "p %2d %2d\n", x, y);
	return msg;
}

void
netmove(int dir)
{
	int len;
	char *msg;

	msg = movemsg(dir);

	if(msg != nil)
	{
		len = strlen(msg);
		if(write(sockfd, msg, len) < len)
			sysfatal("netmove(): half written?");
	}
	else
		sysfatal("netmove(): invalid dir?");
}

void
netput(int x, int y)
{
	int len;
	char *msg;

	msg = putmsg(x, y);
	len = strlen(msg);

	if(write(sockfd, msg, len) < len)
			sysfatal("half written?: %r");

	free(msg);
}

static void
netproc(Netmsg *msg, char *in)
{
	int i = 0, dir;
	char *tmp, *tmparr[2];
	
	char **tokens = malloc(64 * sizeof(char*));;
	Point p;
	
	msg->omsg = strdup(in);
	
	do
	{
		tokens[i] = strtok(in, " ");
	}while(tokens[i++] != nil);
	
	msg->ntoken = i;
	msg->tokens = tokens;
	msg->err = Ok;
	if(!strcmp(tokens[0], "CONN"))
	{
		switch(atoi(tokens[1]))
		{
			case '0':
				pside = PTrapper;
				break;
			case '1':
				pside = PGlenda;
				break;
			default:
				sysfatal("invalid conn");
		}
	}
	else if(!strcmp(tokens[0], "WAIT"))
	{
		waitbit = 1;
	}
	else if(!strcmp(tokens[0], "INIT"))
	{
		waitbit = 0;
		state = Init;
		while(state == Init)
		{
			tmp = netread();
			/* lots of assuming goes here,
			 * messages are in the form of:
			 * {w,g} xx yy\n
			 */
			switch(*tmp)
			{
				case 'w':
					tmparr[0] = getpart(tmp, " ", 1);
					tmparr[1] = getpart(tmp, " ", 2);
					if(tmparr[0] == nil || tmparr[1] == nil)
						sysfatal("netproc(): w tmparr is nil?\n");
					p.x = atoi(tmparr[0]);
					p.y = atoi(tmparr[1]);
					
					grid[p.x][p.y] = Wall;
					break;
				case 'g':
					tmparr[0] = getpart(tmp, " ", 1);
					tmparr[1] = getpart(tmp, " ", 2);
					if(tmparr[0] == nil || tmparr[0])
						dprint("netproc(): g tmparr is nil?\n");
					
					p.x = atoi(tmparr[0]);
					p.y = atoi(tmparr[1]);
					
					grid[p.x][p.y] = Glenda;
					break;
				default:
					if(!strcmp("SENT", tmp))
						state = Start;
					else
						dprint("netproc(): unknown command: %s\n", tmp);
				
			}
		}
	}
	else if(!strcmp(tokens[0], "SENT"))
	{
		/* sent is handled in INIT */
		dprint("SENT without INIT?\n");
	}
	else if(!strcmp(tokens[0], "TURN"))
	{
		waitbit = 0;
	}
	else if(!strcmp(tokens[0], "WALL"))
	{
		msg->err = Wall;
	}
	else if(!strcmp(tokens[0], "GLND"))
	{
		msg->err = Glenda;
	}
	else if(!strcmp(tokens[0], "SYNC"))
	{
		if(msg->ntoken < 2)
			sysfatal("netproc(): not enough toknes?");
		
		if(atoi(msg->tokens[1]) % 2 == 0)
		{
			/* trapper's turn is done */
			if(msg->ntoken != 4)
				sysfatal("netproc(): not enough tokens to SYNC trapper's move");
			
			p = parseput(tokens[2], tokens[3]);
			doput(p);
		}
		else
		{
			/* glenda's turn is done */
			if(msg->ntoken != 3)
				sysfatal("netproc(): not enough tokens to SYNC glenda's move");
			dir = parsemove(tokens[2]);
			domove(dir);
		}
	}
	else if(!strcmp(tokens[0], "WON"))
	{
		msg->err = Won;
	}
	else if(!strcmp(tokens[0], "LOST"))
	{
		msg->err = Lost;
	}
	else 
	{
		sysfatal("netproc(): unkown message");
	}
}

char*
netread(void)
{
	char *s = malloc(1024);
	int n = 0;
	
	memset(s, 0, 1024);
	while(read(sockfd, s+n, 1) == 1 && n < 1024)
	{
		if(s[n] == '\n' || s[n] == '\0')
		{
			s[n] = '\0';
			break;
		}
		n++;
	}
	return s;
}

Netmsg*
netmain(void)
{
	Netmsg *msg;
	char *s;
	msg = malloc(sizeof(Netmsg));

	s = netread();
	netproc(msg, s);
	free(s);
	return msg;
}