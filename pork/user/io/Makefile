include ../../../Makefile.common

INCPATH = -I ../include $(CCINCPATH)

CCPOPTS = -fno-stack-protector -O2 -m32 \
	  -Wall -Wshadow -Wconversion -fno-exceptions

IOOBJS  = get_hex.o print.o ia32-getc.o ia32-putc.o

libio.a: ${IOOBJS}
	ar r libio.a ${IOOBJS}
	ranlib libio.a

print.o:     print.cc
get_hex.o:   get_hex.cc
ia32-getc.o: ia32-getc.cc
ia32-putc.o: ia32-putc.cc

.cc.o:
	gcc -x c++ ${INCPATH} ${CCPOPTS} -o $*.o -c $*.cc

clean:
	-rm -r *.a *.o

