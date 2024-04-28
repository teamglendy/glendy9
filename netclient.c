#include "engine.h"
#includde "netclient.h"

int sockfd;

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
