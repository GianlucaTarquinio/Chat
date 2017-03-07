all: server client

server: server.o chat.o pqueue.o
	gcc server.o chat.o pqueue.o -o server

client: client.o chat.o
	gcc client.o chat.o -o client -lpthread
	
server.o: server.c chat.h pqueue.h
	gcc -c server.c
	
client.o: client.c chat.h
	gcc -c client.c

chat.o: chat.c chat.h
	gcc -c chat.c
	
pqueue.o: pqueue.c pqueue.h
	gcc -c pqueue.c
	
clean:
	rm -rf *.o server client
	