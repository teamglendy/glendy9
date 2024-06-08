typedef struct
{
	char *omsg;
	int ntoken;
	char **tokens;
	int err;
}Netmsg;

extern int srvfd;

extern char *pnick;
extern int pside;
extern int pgame;
extern int popts;

int netmove(int dir);
int netput(int x, int y);
char* netread(void);
void netinit(void);
Netmsg* netmain(void);
