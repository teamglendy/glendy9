</$objtype/mkfile
CFILES=\
	netclient.c\
	engine.c\
	util.c\
	gui9.c\

DOC=\
	doc/glendy2.pdf\
	doc/glendy2.ps\
	doc/glendy2.txt\
	
OFILES=${CFILES:%.c=%.$O}
HFILES=engine.h
BIN=/$objtype/bin
CLEANFILES= doc/*.ps doc/*.pdf cli srv gui9

all:V: ${CFILES}

docs: $DOC
	nroff -man doc/glendy2.man > doc/glendy2.txt
	troff -man doc/glendy2.man | dpost > doc/glendy2.ps
	ps2pdf < doc/glendy2.ps > doc/glendy2.pdf

</sys/src/cmd/mkone
