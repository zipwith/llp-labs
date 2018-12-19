include ../../Makefile.common

all:	libcapio.a

OBJS = printf.o puts.o syscalls.o

libcapio.a: ${OBJS}
	ar r libcapio.a ${OBJS}
	ranlib libcapio.a

#----------------------------------------------------------------------------
# compile the printf implementation:
printf.o: printf.c capio.h
	$(CC) ${CCOPTS} -o printf.o -c printf.c

# compile the puts implementation:
puts.o: puts.c capio.h
	$(CC) ${CCOPTS} -o puts.o -c puts.c

# compile the syscalls:
syscalls.o: syscalls.s
	$(CC) -c -o syscalls.o syscalls.s

#----------------------------------------------------------------------------
# tidy up after ourselves ...
clean:
	-rm -rf libcapio.a ${OBJS}

#----------------------------------------------------------------------------
