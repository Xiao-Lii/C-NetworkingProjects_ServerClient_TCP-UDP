#makefile for ftpc.c

CC = gcc
CFLAGS = -g -Wall

all: $(TARGET)

ftps: 	ftps.c
		$(CC) $(CFLAGS) -o ftps ftps.c 
		ftpc:ftpc.c
		$(CC) $(CFLAGS) -o ftpc ftpc.c
		server.c
		$(CC) $(CFLAGS) -o server server.c
clean:
	rm $(TARGET)