typedef struct
{
	char *omsg;
	int ntoken;
	char **tokens;
	int err;
}Netmsg;

extern int sockfd;

int netmove(int dir);
int netput(int x, int y);
char* netread(void);
Netmsg* netmain(void);