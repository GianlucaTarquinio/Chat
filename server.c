#include <pthread.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <poll.h>

#include "pqueue.h"

#define MSG_LEN 9000
#define MAX_CONNECTIONS 80

typedef struct connectionData {
	pthread_mutex_t lock;
	unsigned valid:1;
	pthread_t thread;
	int i;
	int connection;
} ConnectionData;

ConnectionData connections[MAX_CONNECTIONS];

/** Handle a connection with a client
*
* @param conn file descripter for the socket
*/
void *handleConnection(void *param) {
	ConnectionData *me = (ConnectionData *) param;
	char readBuff[MSG_LEN + 1], writeBuff[MSG_LEN + 1];
	memset(readBuff, '\0', MSG_LEN + 1);
	memset(writeBuff, '\0', MSG_LEN + 1);
	struct pollfd fds[1];
	fds[0].fd = me->connection;
	fds[0].events = POLLIN | POLLPRI | POLLHUP;
	while(1) { 
		//Read from client if there is a message to read
		fds[0].revents = 0;
		if(poll(fds, 1, 0) < 0) {
			printf("poll failed\n");
		} else {
			if(fds[0].revents & POLLHUP) { //Check if socket is closed
				printf("Client disconnected from slot %d\n", me->i);
				close(me->connection);
				pthread_mutex_lock(&(me->lock));
				me->valid = 0;
				pthread_mutex_unlock(&(me->lock));
				return;
			}
			if(fds[0].revents & (POLLIN | POLLPRI)) { //Check if there is data
				if(recv(me->connection, readBuff, MSG_LEN, 0) < 0) {
					printf("recv failed\n");
				} else {
					strncpy(writeBuff, readBuff, MSG_LEN);
					send(me->connection, writeBuff, strnlen(writeBuff, MSG_LEN + 1) + 1, 0);
				}
			}
		}
	}
}

/** Main function for server
*/
int main() {
	//init connection data
	int i;
	for(i = 0; i < MAX_CONNECTIONS; i++) {
		connections[i].valid = 0;
		if(pthread_mutex_init(&(connections[i].lock), NULL)) {
			printf("Initialization failed\n");
			return 1;
		}
	}
	
	int socket_desc = socket(AF_INET, SOCK_STREAM, 0); //Create the socket
	
	//Configure server address struct
	struct sockaddr_in server;
	server.sin_family = AF_INET;
	//My computers ip on my router is 192.168.1.8, port-forwarding port 11777 from my computer for TCP allows
	//connection to the server using the router's ip 74.103.143.15 (./runClient 74.103.143.15:11777) from anywhere
	server.sin_addr.s_addr = INADDR_ANY;//inet_addr("192.168.1.8");
	server.sin_port = htons(11777);
	memset(server.sin_zero, '\0', sizeof(server.sin_zero));
	
	//Bind server struct to socket
	if(bind(socket_desc, (struct sockaddr *) &server, sizeof(server)) < 0) {
		printf("Bind error\n");
		close(socket_desc);
		return 1; 
	}
	
	//Listen on socket, with up to 7 connection requests queued
	if(listen(socket_desc, 7) == 0) {
		//printf("Listening\n");
	} else {
		printf("Error in listen\n");
		return 1;
	}
	
	printf("Server started\n");
	
	//Accept connections
	int conn, process, found;
	while(1) {
		if((conn = accept(socket_desc, (struct sockaddr *) NULL, NULL)) >= 0) {
			i = 0;
			found = 0;
			while(!found && i < MAX_CONNECTIONS) {
				pthread_mutex_lock(&(connections[i].lock));
				if(!connections[i].valid) {
					pthread_mutex_unlock(&(connections[i].lock));
					connections[i].valid = 1;
					connections[i].connection = conn;
					connections[i].i = i;
					if(pthread_create(&(connections[i].thread), NULL, handleConnection, connections + i)) {
						printf("Error creating pthread\n");
						close(conn);
						connections[i].valid = 0;
					} else {
						printf("Client connedcted to slot %d\n", i);
					}
					found = 1;
				} else {
					pthread_mutex_unlock(&(connections[i].lock));
					i++;
				}
			}
			if(!found) {
				close(conn);
			}
		}
	}
	
	return 0;
}
