include ../../../Makefile.common

INCPATH = -I ../../../simpleio \
	  -I ../include \
	  $(CCINCPATH)


LIBPATH = -L ../lib -ll4 \
	  -L ../../../simpleio -lio

#-L ../io -lio

CCPOPTS = -fno-stack-protector -nostdinc -g -O2 -m32 \
	  -Wall -Wshadow -Wconversion -fno-exceptions

PINGPONG_OBJS  = crt0-ia32.o pingpong.o

all:		pingpong

#pingpong.s:	pingpong.cc
#	gcc -m32 -x c++ ${INCPATH} ${CCOPTS} -S $*.cc

pingpong:	${PINGPONG_OBJS} pingpong.ld
	$(LD) -T pingpong.ld -o pingpong ${PINGPONG_OBJS} ${LIBPATH} \
		--print-map > pingpong.map

crt0-ia32.o:	crt0-ia32.S

pingpong.o:	pingpong.cc ia32.h

.S.o:
	$(CC) -o $*.o -c $*.S
.cc.o:
	gcc -m32 -x c++ ${INCPATH} ${CCPOPTS} -o $*.o -c $*.cc

clean:
	-rm -r *.o pingpong pingpong.map

