CC=cc
CFLAGS=-g -O0
L=\
	unix.o\
	engine.o\

all: cli

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $< -o $@

cli: ${L}
	$(CC) $(CFLAGS) -o $@ ${L} cli.c

srv: ${L}
	$(CC) ${CFLAGS} -o $@ ${L} srv.c

glendy-cli: cli.o ${L}
	$(LD) $(LDFLAGS) -o $@ ${L} cli.o

clean:
	rm -f a.out *.8 *.6 *.o cli sdl srv ${L}
