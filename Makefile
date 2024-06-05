CC=cc
CFLAGS +=-g -Dunix -pthread
NROFF=mandoc

Lib=\
	engine.o\
	unix.o\
	util.o\
	netclient.o\

Cli=	cli.o
Srv5=	srv5.o
Srv4=	srv4.o

all:
	@echo 'make cli, clinet, srv5, srv4 or docs'

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $< -o $@

cli: ${Cli} ${Lib} netclient.o
	$(CC) $(CFLAGS) -o $@ ${Cli} ${Lib}

clinet: clinet.o ${Lib}	netclient.o
	$(CC) $(CFLAGS) -o $@ clinet.o ${Lib}

srv4: ${Srv4} ${Lib}
	$(CC) $(CFLAGS) -o $@ ${Srv4} ${Lib}

srv5: ${Srv5} ${Lib}
	$(CC) $(CFLAGS) -o $@ ${Srv5} ${Lib}
docs:
	$(NROFF) doc/glendy2.man | col -b > doc/glendy2.txt
clean:
	rm -f *.o *.5 *.6 *.7 *.8 cli srv srv4 doc/glendy2.txt
