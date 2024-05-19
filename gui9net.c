#include <u.h>
#include <libc.h>
#include <draw.h>
#include <event.h>
#include "netclient.h"
#include "engine.h"

Font *font;

Image	*gl;	/* glenda */
Image 	*glm;	/* glenda's mask */
Image	*cc; /* clicked */
Image	*ec; /* empty; not clicked */
Image 	*bg;
Image 	*lost;
Image	*won;

int debug;
char *server = nil;

char *mbuttons[] = 
{
	"Easy",
	"Medium",
	"Hard",
	"Impossible",
	0
};

char *rbuttons[] = 
{
	"New",
	"Undo",
	"Reset",
	"Exit",
	0
};

Menu mmenu = 
{
	mbuttons,
};

Menu rmenu =
{
	rbuttons,
};

Image *
eallocimage(Rectangle r, int repl, uint color)
{
	Image *tmp;

	tmp = allocimage(display, r, screen->chan, repl, color);
	if(tmp == nil)
		sysfatal("cannot allocate buffer image: %r");

	return tmp;
}

Image *
eloadfile(char *path)
{
	Image *img;
	int fd;

	fd = open(path, OREAD);
	if(fd < 0) {
		fprint(2, "cannot open image file %s: %r\n", path);
		exits("image");
	}
	img = readimage(display, fd, 0);
	if(img == nil)
		sysfatal("cannot load image: %r");
	close(fd);
	
	return img;
}

void
allocimages(void)
{
	Rectangle one = Rect(0, 0, 1, 1);
	
	cc = eallocimage(one, 1, DGreyblue);
	ec = eallocimage(one, 1, DPalebluegreen);
	bg = eallocimage(one, 1, DPaleyellow);
	lost = eallocimage(one, 1, DRed);
	won = eallocimage(one, 1, DPalegreen);
	gl = eloadfile("/lib/face/48x48x4/g/glenda.1");

	glm = allocimage(display, Rect(0, 0, 48, 48), gl->chan, 1, DCyan);
	if(glm == nil)
        		sysfatal("cannot allocate mask: %r");

    	draw(glm, glm->r, display->white, nil, ZP);
    	gendraw(glm, glm->r, display->black, ZP, gl, gl->r.min);
    	freeimage(gl);
    	gl = display->black;


}

Point
pix2board(int x, int y)
{
	float d, rx, ry, yh;
	int ny, nx;

	/* XXX: float→int causes small rounding errors */

	d = (float)(Dx(screen->r) > Dy(screen->r)) ? Dy(screen->r) -20: Dx(screen->r)-20;
	rx = d/(float)SzX;
	rx = rx/2.0;
	ry =d/(float)SzY;
	ry = ry/2.0;

	yh = ry/3.73205082;

	/* reverse board2pix() */
	ny = (int)(((float)y - ry)/(2*ry - ((y>2*ry)?yh:0.0)) + 0.5); /* ny = (y - ry)/(2ry-yh) */
	nx = (int)(((float)x - rx - (ny%2?rx:0.0))/(rx*2.0) + 0.5); /* nx = (x - rx - rx)/2rx */
	
	if (nx >= SzX)
		nx = SzX-1;
	if (ny >=SzY)
		ny = SzY-1;

	return Pt(nx, ny);
}

/* unnecessary calculations here, but it's fine */
Point
board2pix(int x, int y)
{
	float d, rx, ry, yh;
	int nx, ny;

	d = (float)(Dx(screen->r) > Dy(screen->r)) ? Dy(screen->r) -20 : Dx(screen->r) -20;
	rx = d/(float)SzX;
	rx = rx/2.0;
	ry = d/(float)SzY;
	ry = ry/2.0;

	yh = ry/3.73205082;

	nx = (int)((float)x*rx*2.0+rx +(y%2?rx:0.0)); /* nx = x*(2rx) + rx + rx (conditional) */
	ny = (int)((float)y*(ry*2.0-(y>0?yh:0.0)) + ry); /* ny = y*(2ry-yh) +ry */
	return Pt(nx, ny);
}

void
drawlevel(void)
{
	Point p;
	int  x, y, rx, ry, d;
	char *s = nil;

	if(state == Won)
		draw(screen, screen->r, won, nil, ZP);
	else if(state == Lost)
		draw(screen, screen->r, lost, nil, ZP);
	else
		draw(screen, screen->r, bg, nil, ZP);

	d = (Dx(screen->r) > Dy(screen->r)) ? Dy(screen->r) -20: Dx(screen->r) -20;
	rx = (int)ceil((float)(d-2*Border)/(float)SzX)/2;
	ry = (int)ceil((float)(d-2*Border)/(float)SzY)/2;

	for(x = 0; x < SzX; x++) {
		for(y = 0; y < SzY; y++)
		{
			p = board2pix(x, y);
			switch(grid[x][y])
			{
				case Wall: 
					fillellipse(screen, addpt(screen->r.min, p), rx, ry, cc, ZP);
					break;
				case Glenda:
					p = addpt(screen->r.min, p);
					fillellipse(screen, p, rx, ry, ec, ZP);
					p = subpt(p, Pt(24, 24));
					draw(screen, Rpt(p, addpt(p, Pt(48, 48))), gl, glm, ZP);
					break;
				default:
					fillellipse(screen, addpt(screen->r.min, p), rx, ry, ec, ZP);
					USED(s);
					if(debug)
					{
						s = smprint("%d", grid[x][y]);
						string(screen, addpt(screen->r.min, p), display->black, ZP, font, s);
						free(s);
					}
	
					break;
			}
		}
	}
	flushimage(display, 1);
}
void
move(Point m)
{
	int dir;
	Point g, p, nm;

	nm = subpt(m, screen->r.min);

	/* figure out where the click falls */
	p = pix2board(nm.x, nm.y);
	g = findglenda();
	
	dir = pointdir(g, p);
	
	if(grid[p.x][p.y] >= 999 || dir == Err)
		return;

	/* find the direction to p from our currently pos, then move */
	domove(dir);
}

void
put(Point m)
{
	Point p, nm;

	nm = subpt(m, screen->r.min);

	/* figure out where the click falls */
	p = pix2board(nm.x, nm.y);
	
	if(grid[p.x][p.y] >= 999)
		return;

	doput(p);
}

void
resize(void)
{
	int fd, size = (Dx(screen->r) > Dy(screen->r)) ? Dy(screen->r) + 20 : Dx(screen->r)+20; 

	fd = open("/dev/wctl", OWRITE);
	if(fd >= 0)
	{
		fprint(fd, "resize -dx %d -dy %d", size, size);
		close(fd);
	}
}

void
eresized(int new)
{
	if(new && getwindow(display, Refnone) < 0)
		sysfatal("can't reattach to window");
	
	drawlevel();
}

void
usage(void)
{
	fprint(2, "usage: %s [-dg] [-n server]\n", argv0);
	exits("usage");
}

void 
main(int argc, char **argv)
{
	Mouse m;
	Event ev;
	Netmsg *msg;
	int e, mousedown=0;

	/* todo, add flags for human playing */
	ARGBEGIN{
		case 'D':
			debug++;
			break;
		case 'd':
			//ptype[0] = Computer /* todo */
			sysfatal("No computer player for defenders yet");
			break;
		case 'g':
			ptype[1] = Computer;
			break;
		case 'n':
			ptype[0] = Net;
			ptype[1] = Net;
			networked = 1;
			server = EARGF(usage());
			break;
		default:
			usage();
	}ARGEND
	if(initdraw(nil, nil, "glendy") < 0)
		sysfatal("initdraw failed: %r");
	einit(Emouse);

	if(server != nil && networked)
	{
		srvfd = dial(server, nil, nil, nil);
		if(srvfd < 0)
			sysfatal("unable to connect: %r");
	}
	
	resize();

	srand(time(0));

	allocimages();
//	initlevel();	/* must happen before "eresized" */
	eresized(0);

	if(networked)
	{
		netmain(); /* CONN */
		netmain(); /* SYNC */
		netmain(); /* TURN/WAIT */
	}
	for(;;)
	{
		if(networked && waitbit && isplaying)
		{
			msg = netmain();
			if(msg->tokens[0] != nil && strcmp(msg->tokens[0], "SYNC"))
				drawlevel();
		}
		e = event(&ev);
		switch(e)
		{
			case Emouse:
				m = ev.mouse;
				if(m.buttons == 0)
				{
					if(mousedown && isplaying)
					{
						mousedown = 0;
						if(turn % 2 == 0)
							put(m.xy);
						else
							move(m.xy);
						if(!networked)
							drawlevel();
					}
				}
				if(m.buttons&1)
				{
					mousedown = 1;
				}
				if(m.buttons&2)
				{
					switch(emenuhit(2, &m, &mmenu))
					{
					case 0:
						difficulty = DEasy;
						initlevel();
						break;
					case 1:				
						difficulty = DMed;
						initlevel();
						break;
					case 2:
						difficulty = DHard;
						initlevel();
						break;
					case 3:
						difficulty = DImp;
						initlevel();
						break;
					}
					drawlevel();
				}
				if(m.buttons&4) {
					switch(emenuhit(3, &m, &rmenu))
					{
						case New:
							initlevel();
							break;
						case Undo:
							undo();
							break;
						case Restart:
							restart();
							break;
						case Exit:
							exits(nil);
					}
					drawlevel();
				}
				break;
			}
	}
}
