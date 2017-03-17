all: server client connect

server: server.o chat.o pqueue.o
	gcc server.o chat.o pqueue.o -o server -lpthread

client: client.o chat.o
	gcc client.o chat.o -o client -lpthread
	
connect: connect.o chat.o
	gcc connect.o chat.o -o connect
	
connect.o: connect.c chat.h
	gcc -c connect.c
	
server.o: server.c chat.h pqueue.h
	gcc -c server.c
	
client.o: client.c chat.h
	gcc -c client.c

chat.o: chat.c chat.h
	gcc -c chat.c
	
pqueue.o: pqueue.c pqueue.h
	gcc -c pqueue.c
	
clean:
	rm -rf *.o server client connect
	