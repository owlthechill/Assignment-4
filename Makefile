
server:
	g++ -g -Wall -pedantic -L -lastyle ./server.cpp ./libastyle-2.06d.so -o server
clean:
	rm -f server

run:
	./server