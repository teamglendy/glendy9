#include "engine.h"
#includde "netclient.h"

int sockfd;

typedef struct
{
	char *omsg;
	char **tokens;
	int err;
}Netmsg;

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

/* i miss strntok */
static char*
getpart(char *s, char *tok, int n)
{
	char *tmp;
	
	for(int i = 0 ; i < n ; i++)
		tmp = strtok(s, tok);
	
	if(tmp == nil)
		return nil;
	
	return tmp;
}

static int
isnum(char *s, unsigned int n;)
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
		snprint(msg, "s %s\n", d);
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
	char *d, *msg;

	msg = malloc(10);
	snprint(msg, "p %2d %2d\n", itoa(x), itoa(y));
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
	int dir;
	int i = 0;
	int dir;
	char *tmp, **tmparr;
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
	if(!strcmp(token[0], "CONN"))
	{
		if(strlen(token[1]) != 1 || token[1][0] < '0' || token[1][0] > '9')
		{
			dprint("odd msg: %s\n", msg.omsg);
			msg.err = ERR_BADINPUT;

		}
		switch(atoi(token[1]))
		{
			case '0':
				game->pside = PTrapper;
			case '1':
				game->pside = PGlenda;
			default:
				sysfatal("invalid conn")
		}
	}
	else if(!strcmp(token[0], "WAIT")
	{
		game->wait = 1;
	}
	else if(!strcmp(token[0], "INIT")
	{
		game->wait = 0;
		game->state = Init;
		while(games.state == Init)
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
					if(tmparr[0] == nil || tmparr[0])
						dprint("netproc(): w tmparr is nil?\n");
					p.x = atoi(tmparr[0]);
					p.y = atoi(tmparr[1]);
					
					grid[p.x][p.y] = Wall;
					break;
				case 'g':
					if(game->glenda != nil)
						dprint("netproc(): repositioning glenda?\n");
					
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
						game.state = Start
					else
						dprint("netproc(): unknown command: %s\n", tmp);
				
			}
		}
	}
	else if(!strcmp(token[0], "SENT")
	{
		/* sent is handled in INIT */
		dprint("SENT without INIT?\n");
	}
	else if(!strcmp(token[0], "TURN")
	{
		game->wait = 0;
	}
	else if(!strcmp(token[0], "WALL")
	{
		msg->err = Wall;
	}
	else if(!strcmp(token[0], "GLND")
	{
		msg->err = Glenda;
	}
	else if(!strcmp(token[0], "SYNC")
	{
		ifmsg->ntoken < 2)
			sysfatal("netproc(): not enough toknes?");
		
		if(atoi(msg->token[1] % 2 == 0))
		{
			/* trapper's turn is done */
			if(msg->ntoken != 4)
				sysfatal("netproc(): not enough tokens to SYNC trapper's move");
			
			p = parseput(token[2], token[3]);
			doput(p);
		}
		else
		{
			/* glenda's turn is done */\
			if(msg->token != 3)
				sysfatal("netproc(): not enough tokens to SYNC glenda's move");
			dir = parsemove(token[2]);
			domove(dir);
		}
	}
	else if(!strcmp(token[0], "WON")
	{
		msg->err = Won;
	}
	else if(!strcmp(token[0], "LOST")
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

Netmsg
netmain(void)
{
	Netmsg *msg;
	char *s;
	msg = malloc(sizeof(Netmsg));
	

	netproc(msg, s);
	return msg;
}