include ../Makefile.common

all:	libwinio.a

libwinio.a: winio.o serial.o ia32-getc.o
	ar r libwinio.a winio.o serial.o ia32-getc.o
	ranlib libwinio.a

#----------------------------------------------------------------------------
# compile the winio library:
winio.o: winio.c winio.h
	$(CC) ${CCOPTS} -o winio.o -c winio.c

# compile the getc code:
ia32-getc.o: ia32-getc.c
	$(CC) ${CCOPTS} -o ia32-getc.o -c ia32-getc.c

# compile the serial output code:
serial.o: serial.s
	$(CC) -c -o serial.o serial.s

#----------------------------------------------------------------------------
# tidy up after ourselves ...
clean:
	-rm -rf winio.o serial.o ia32-getc.o libwinio.a

#----------------------------------------------------------------------------
