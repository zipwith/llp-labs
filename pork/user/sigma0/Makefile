include ../../../Makefile.common

INCPATH = -I ../../../simpleio -I ../include
LIBPATH = -L ../../../simpleio -lio -L ../lib -ll4

all:	sigma0

#----------------------------------------------------------------------------
# A simple sigma0 implementation:
OBJS	= sigma0.o sigma0boot.o
sigma0:	${OBJS} sigma0.ld
	$(LD) -T sigma0.ld -o sigma0 ${OBJS} ${LIBPATH} --print-map > sigma0.map
	strip sigma0

sigma0.o: sigma0.c
	$(CC) ${CCOPTS} ${INCPATH} -o sigma0.o -c sigma0.c

sigma0boot.o: sigma0boot.s
	$(CC) -Wa,-alsm=sigma0boot.lst -c -o sigma0boot.o sigma0boot.s

#----------------------------------------------------------------------------
# tidy up after ourselves ...
clean:
	-rm -r sigma0 *.o *.lst *.map

#----------------------------------------------------------------------------
