#ifdef unix
#include "unix.h"
#else
#include <u.h>
#include <libc.h>
#include <draw.h>
#endif
#include "engine.h"
#include "netclient.h"
#include "util.h"

int waitbit = 1; /* 0 is go, 1 is wait */
int networked = 0; /* 0 is local, 1 is networked */
int pside; /* Trapper, Glenda */

int srvfd;

char *pnick;
int pside;
int pgame;
int popts;

static char*
himsg(char *name, int game, int side, int opts)
{
	char *msg;
	
	dprint("himsg(%s, %d, %d, %d)\n", name, game, side, opts);
	if(side > PRandom)
		return nil;
	
	msg = (char*)emalloc(32);
	sprint(msg, "%s %d %d %d\n", name, game, side, opts);
	return msg;
}

static char*
movemsg(int dir)
{
	char *d, *msg;
	
	d = dirtostr(dir);
	if(d == nil)
		return nil;
	
	msg = (char*)emalloc(8);
	sprint(msg, "m %s\n", d);
	return msg;
}

static char*
putmsg(int x, int y)
{
	char *msg;
	dprint("putmsg(%d, %d)\n", x, y);
	if(x > SzX || x < 0 || y > SzY || y < 0)
		return nil;
	
	msg = (char*)emalloc(10);
	sprint(msg, "p %d %d\n", x, y);
	return msg;
}

int
nethi(char *name, int game, int side, int opts)
{	
	int len;
	char *msg;

	msg = himsg(name, game, side, opts);
	if(msg == nil)
		return Err;
	
	len = strlen(msg);
	if(write(srvfd, msg, len) < len)
		sysfatal("nethi(): half written?");

	/* otherwise client wont read socket to confirm */
	waitbit = 1;
	free(msg);
	state = Init;
	return Ok;
}

int
netmove(int dir)
{
	int len;
	char *msg;

	msg = movemsg(dir);

	if(msg == nil)
		return Err;
	
	len = strlen(msg);
	if(write(srvfd, msg, len) < len)
		sysfatal("netmove(): half written?");

	/* otherwise client wont read socket to confirm */
	waitbit = 1;
	free(msg);
	return Ok;
}

int
netput(int x, int y)
{
	int len;
	char *msg;

	msg = putmsg(x, y);
	
	if(msg == nil)
		return Err;
	
	len = strlen(msg);
	if(write(srvfd, msg, len) < len)
			sysfatal("netput(): half written?: %r");

	/* otherwise client wont read socket to confirm */
	waitbit = 1;
	free(msg);
	return Ok;
}

static void
netproc(Netmsg *msg, char *in)
{
	int i, dir;
	char *tmp, *xpos, *ypos;
	char **tokens = (char**)emalloc(64 * sizeof(char*));
	Point p;
	
	msg->omsg = strdup(in);
	dprint("msg->omsg: %s\n", in);
	
	tokens[0] = strtok(in, " ");
	for(i = 1 ; tokens[i-1] != nil ; i++)
		tokens[i] = strtok(nil, " ");
	
	msg->ntoken = i;
	msg->tokens = tokens;
	msg->err = Ok;

	if(tokens[0] == nil)
		msg->err = Err;
	else if(!strcmp(tokens[0], "CONN"))
	{
		switch(*tokens[1])
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
	else if(!strcmp(tokens[0], "UGUD"))
		fprint(srvfd, "y\n");

	else if(!strcmp(tokens[0], "WAIT"))
		waitbit = 1;

	else if(!strcmp(tokens[0], "INIT"))
	{
		dprint("INIT XXX in INIT\n");
		state = Init;
		while(state == Init)
		{
			tmp = netread();
			/* lots of assuming goes here,
			 * messages are in the form of:
			 * {w,g} xx yy\n
			 */
			strtok(tmp, " ");
			switch(*tmp)
			{
				case 'w':
					xpos = strtok(nil, " ");
					ypos = strtok(nil, " ");
					if(xpos == nil || ypos == nil)
						sysfatal("netproc(): w xpos or ypos is nil?\n");
					p.x = atoi(xpos);
					p.y = atoi(ypos);
					
					grid[p.x][p.y] = Wall;
					break;
				case 'g':
					xpos = strtok(nil, " ");
					ypos = strtok(nil, " ");
					if(xpos == nil || ypos == nil)
						dprint("netproc(): g xpos or ypos is nil?\n");
					
					p.x = atoi(xpos);
					p.y = atoi(ypos);
					
					grid[p.x][p.y] = Glenda;
					break;
				default:
					if(!strcmp("SENT", tmp))
						state = Start;
					else
						dprint("netproc(): Init: unknown command: %s\n", tmp);
				
			}
		}
	dprint("XXX INIT: state = %d\n", state);
	}
	else if(!strcmp(tokens[0], "SENT"))
		/* sent is handled in INIT */
		dprint("SENT without INIT?\n");
	
	else if(!strcmp(tokens[0], "TURN"))
		waitbit = 0;

	else if(!strcmp(tokens[0], "SYNC"))
	{
		if(state == Start)
			state = Playing;
		if(msg->ntoken < 2)
			sysfatal("netproc(): not enough toknes?");
		
		/* TODO: very ugly hack, get rid of this */
		networked = 0;
		if(atoi(msg->tokens[1]) % 2 == 0)
		{
			/* glenda's turn is done */
			if(msg->ntoken < 3)
				sysfatal("netproc(): not enough tokens to SYNC glenda's move");
			dir = strtodir(tokens[2]);
			domove(dir);
			dprint("in glenda's turn\n");
		}
		else
		{
			/* trapper's turn is done */
			if(msg->ntoken < 4)
				sysfatal("netproc(): not enough tokens to SYNC trapper's move");
			
			p = parseput(tokens[2], tokens[3]);
			doput(p);
		}
		/* TODO: very ugly hack, get rid of this */
		networked = 1;
	}
	else if(!strcmp(tokens[0], "WON"))
		state = Won;
	
	else if(!strcmp(tokens[0], "LOST"))
		state = Lost;
	else if(!strcmp(tokens[0], ""))
		state = Finished;
	else 
		sysfatal("netproc(): unkown message: 0x%x %s", *tokens[0], tokens[0]);
}

char*
netread(void)
{
	char *s = malloc(1024);
	int n = 0;
	
	memset(s, 0, 1024);
	while(read(srvfd, s+n, 1) == 1 && n < 1023)
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

void
netinit(void)
{
	Netmsg *msg;
	char *s;

	msg = (Netmsg*)emalloc(sizeof(Netmsg));

	state = Connect;
	nethi(pnick, 0, pside, 0);
	
	for(;;)
	{
		s = netread();
		netproc(msg, s);

		dprint("msg->tokens[0]: %s\n", msg->tokens[0]);

		if(msg->tokens[0] != nil && !strcmp(msg->tokens[0], "INIT"))
			break;
		free(s);
	}
	free(s);
}

Netmsg*
netmain(void)
{
	Netmsg *msg;
	char *s;
	msg = (Netmsg*)emalloc(sizeof(Netmsg));
	
	s = netread();
	netproc(msg, s);
	free(s);
	return msg;
}
