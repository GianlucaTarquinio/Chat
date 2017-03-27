all: server client connect

mac-apps: mac-client mac-server

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
	
mac-client: mac_app_structure/GChatClient.app connect client
	mkdir -p mac_apps
	cp -R mac_app_structure/GChatClient.app mac_apps
	cp connect client save.txt mac_apps/GChatClient.app/Contents/MacOS

mac-server: mac_app_structure/GChatServer.app server
	mkdir -p mac_apps
	cp -R mac_app_structure/GChatServer.app mac_apps
	cp server mac_apps/GChatServer.app/Contents/MacOS

clean:
	rm -rf *.o server client connect

clean-all:
	make clean
	rm -rf mac_apps
	