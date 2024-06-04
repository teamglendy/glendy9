#include "port.h"

#include "util.h"
#include "engine.h"

int
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

int
strtodir(char *s)
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
		return Err;

	return d;
}

char*
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

/* xx yy\0
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

Point
parseput(char *x, char *y)
{
	if(isnum(x, 2) != 1 && isnum(y, 2) != 1)
		sysfatal("parseput(): input isnt a number?");
	
	return Pt(atoi(x), atoi(y));
}

int
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

void*
emalloc(unsigned long n)
{
	void *p;

	p = malloc(n);
	if(p == nil)
		sysfatal("malloc: %r");
	
	return p;	
}
/* list */
int
lladd(List *last, void *data)
{
	List *temp;

	dprint("lladd(%p, %p)\n", last, data);
	
	if(last->next != nil)
		return 0;

	dprint("lladd(): adding entry\n");
	
	temp = llnew();
	
	last->next = temp;
	temp->data = data;
	
	return 1;
}

void
llappend(List *first, void *data)
{
	List *temp, *last;

	dprint("lladd(%p, %p)\n", first, data);

	/* workaround for first entry */	
	if(first->data == nil)
		temp = first;
	else
		temp = llnew();
	temp->data = data;
	
	last = first->prev;
	if(last != nil)
		last->next = temp;
	
	first->prev = temp;
}

List*
llnew(void)
{
	List *l;
	l = (List*)emalloc(sizeof(List));
	l->next = nil;
	l->data = nil;
	l->prev = nil;
	
	return l;
}

/* returns nth item from list  */
void*
lookup(List *l, int n)
{
	List *tmp;
	
	dprint("lookup(%p, %d)\n", l, n);
	/* cycles thru list if n > list size */
	for(tmp = l ; --n > 0 ; tmp = tmp->next)
		;
	
	return tmp->data;
}

/* quene */
void
qadd(Quene *q, void *data)
{
	q->len++;
	llappend(q->l, data);
}

void
qnext(Quene *q)
{
	List *oldfirst, *last;
	if(q->len == 0)
		sysfatal("qnext(): q->len == 0");
	
	q->len--;
	
	/* note the last node */
	last = q->l->prev;
	
	/* note the first */
	oldfirst = q->l;
	
	/* delete oldfirst */
	q->l = q->l->next;
	free(oldfirst);
	
	/* update last */
	last->next = q->l;
}
