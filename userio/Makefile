include ../Makefile.common

all:	libuserio.a

OBJS = printf.o puts.o syscalls.o

libuserio.a: ${OBJS}
	ar r libuserio.a ${OBJS}
	ranlib libuserio.a

#----------------------------------------------------------------------------
# compile the printf implementation:
printf.o: printf.c userio.h
	$(CC) ${CCOPTS} -o printf.o -c printf.c

# compile the puts implementation:
puts.o: puts.c userio.h
	$(CC) ${CCOPTS} -o puts.o -c puts.c

# compile the syscalls:
syscalls.o: syscalls.s
	$(CC) -c -o syscalls.o syscalls.s

#----------------------------------------------------------------------------
# tidy up after ourselves ...
clean:
	-rm -rf libuserio.a ${OBJS}

#----------------------------------------------------------------------------
