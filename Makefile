CC=g++
CFLAG=-g -Wall -pedantic -L -lastyle-2.06d

server:
	$(CC) $(CFLAG) -o server server.cpp

clean:
	rm -f server

run:
	./server