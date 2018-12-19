include ../../../Makefile.common

INCPATH = -I ../include \
	  $(CCINCPATH)

CCPOPTS = -fno-stack-protector -nostdinc -g -O2 -m32 \
	  -Wall -Wshadow -Wconversion -fno-exceptions

SIGMA0_OBJS  = region.o sigma0_mem.o sigma0.o

all:		${SIGMA0_OBJS}

region.o:	region.cc
sigma0.o:	sigma0.cc
sigma0_mem.o:	sigma0_mem.cc
sigma0_io.o:	sigma0_io.cc

.cc.o:
	gcc -x c++ ${INCPATH} ${CCPOPTS} -o $*.o -c $*.cc

clean:
	-rm -r *.a *.o

#===> serv/sigma0/crt0-ia32.S
#gcc -I../../../user/include -I../.. -I/usr/lib/gcc/i486-linux-gnu/4.2.4/include   -fno-stack-protector -nostdinc -g -O2 -m32 -Wall -Wshadow -Wconversion  -c ../../../user/serv/sigma0/crt0-ia32.S -o crt0-ia32.o

# ===> Linking serv/sigma0/sigma0
# ld -e_start -N -L../../lib -L/usr/lib/gcc/i486-linux-gnu/4.2.4 -nostdlib  -melf_i386  -Ttext=00020000 crt0-ia32.o sigma0_io.o sigma0.o sigma0_mem.o region.o    -ll4 -lio -lgcc -o sigma0

