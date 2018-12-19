include ../../../Makefile.common

INCPATH = -I ../../../simpleio -I ../include
LIBPATH = -L ../../../simpleio -lio -L ../lib -ll4

all:	root

#----------------------------------------------------------------------------
# A simple root task:
OBJS	= root.o showkip.o rootboot.o
root:	${OBJS} root.ld
	$(LD) -T root.ld -o root ${OBJS} ${LIBPATH} --print-map > root.map
	strip root

root.o: root.c
	$(CC) ${CCOPTS} ${INCPATH} -o root.o -c root.c

showkip.o: showkip.c
	$(CC) ${CCOPTS} ${INCPATH} -o showkip.o -c showkip.c

rootboot.o: rootboot.s
	$(CC) -Wa,-alsm=rootboot.lst -c -o rootboot.o rootboot.s

#----------------------------------------------------------------------------
# tidy up after ourselves ...
clean:
	-rm -r root *.o *.lst *.map

#----------------------------------------------------------------------------
