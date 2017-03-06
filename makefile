all: server client

server: server.o pqueue.o
	gcc server.o pqueue.o -o server

client: client.o
	gcc client.o -o client -lpthread
	
server.o: server.c pqueue.h constants.h
	gcc -c server.c
	
client.o: client.c constants.h
	gcc -c client.c
	
pqueue.o: pqueue.c pqueue.h
	gcc -c pqueue.c
	
clean:
	rm -rf *.o server client
	