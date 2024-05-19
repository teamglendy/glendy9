CC=cc
CFLAGS +=-Dunix -pthread

Lib=\
	engine.o\
	unix.o\
	util.o\
	netclient.o\

Cli=	cli.o
Srv4=	srv4.o
Srv=	srv.o

all: cli srv4

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $< -o $@

cli: ${Cli} ${Lib}
	$(CC) $(CFLAGS) -o $@ ${Cli} ${Lib}

srv4: ${Srv4} ${Lib}
	$(CC) $(CFLAGS) -o $@ ${Srv4} ${Lib}

srv: ${Srv} ${Lib}
	$(CC) $(CFLAGS) -o $@ ${Srv} ${Lib}

clean:
	rm -f *.o *.5 *.6 *.7 *.8 cli srv
