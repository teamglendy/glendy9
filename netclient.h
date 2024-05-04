typedef struct
{
	char *omsg;
	int ntoken;
	char **tokens;
	int err;
}Netmsg;

extern int sockfd;

void netmove(int dir);
void netput(int x, int y);
char* netread(void);
Netmsg* netmain(void);