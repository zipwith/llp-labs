include ../../Makefile.common

INCPATH = -I ../capio
LIBPATH = -L ../capio -lcapio

all:	user

#----------------------------------------------------------------------------
# A simple user program:
UOBJS	= user.o userlib.o
user:	${UOBJS} user.ld
	$(LD) -T user.ld -o user ${UOBJS} ${LIBPATH} --print-map > user.map
	strip user

user.o:	user.c syscalls.h
	$(CC) ${CCOPTS} ${INCPATH} -o user.o -c user.c

userlib.o: userlib.s
	$(CC) -Wa,-alsm=userlib.lst -c -o userlib.o userlib.s

#----------------------------------------------------------------------------
# tidy up after ourselves ...
clean:
	-rm -r user *.o *.lst *.map

#----------------------------------------------------------------------------
