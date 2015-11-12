CC = gcc
CFLAGS = -std=c99 -g3 -Wall -pedantic
HWK = /c/cs323/Hwk4

all: encode decode

lzwHashtable.o: lzwHashTable.h lzwHashTable.c

encode: lzw.c lzw.h code.o lzwHashTable.o fcode.o
	${CC} ${CFLAGS} -o $@ $^

decode: encode
	ln -f encode decode

${HWK}/code.o: code.c code.h

fcode.o: fcode.c code.h fcode.h

clean:
	$(RM) encode decode *.o