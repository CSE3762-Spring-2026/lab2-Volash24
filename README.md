[![Review Assignment Due Date](https://classroom.github.com/assets/deadline-readme-button-22041afd0340ce965d47ae6ef1cefeee28c7c493a6346c4f15d667ab976d596c.svg)](https://classroom.github.com/a/Nqgg2i6Z)
CC=gcc
CFLAGS=-Wall -g
LIBS=-lcjson

all: server2 client2

server2: server2.c
$(CC) $(CFLAGS) -o server2 server2.c $(LIBS)

client2: client2.c
$(CC) $(CFLAGS) -o client2 client2.c $(LIBS)

clean:
rm -f server2 client2 *.o
