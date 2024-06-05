typedef struct List List;
struct List
{
	void *data;
	List *next;
};

typedef struct
{
	int len;
	List *head;
	List *tail;
}Quene;

int isnum(char *s, unsigned int n);
int strtodir(char *s);
char* dirtostr(int dir);
Point parseput(char *x, char *y);
int dprint(char *fmt, ...);
void* emalloc(unsigned long n);

/* ll */
int lladd(List *tail, void *data);
List* llnew(void);
void llappend(List *first, void *data);
void* lookup(List *l, int n);

/* quene */
void qadd(Quene *q, void *data);
void qnext(Quene *q);
