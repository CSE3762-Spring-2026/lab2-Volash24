CC = gcc
CFLAGS = -Wall -g
LIBS = -lcjson


server2: server2.c
	$(CC) $(CFLAGS) -o server2 server2.c $(LIBS)

client2: client2.c
	$(CC) $(CFLAGS) -o client2 client2.c $(LIBS)

clean:

	rm -f server2 client2 *.o

