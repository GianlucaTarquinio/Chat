all: server client

server: server.o pqueue.o
	gcc server.o pqueue.o -o server

client: client.o
	gcc client.o -o client -lpthread
	
server.o: server.c pqueue.h
	gcc -c server.c
	
client.o: client.c
	gcc -c client.c
	
queue.o: queue.c pqueue.h
	gcc -c pqueue.c
	
clean:
	rm -rf *.o server client
	