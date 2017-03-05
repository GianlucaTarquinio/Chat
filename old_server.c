#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <poll.h>

#include "pqueue.h"

#define MSG_LEN 9000

/** Handle a connection with a client
*
* @param conn file descripter for the socket
*/
void handleConnection(int conn) {
	char readBuff[MSG_LEN + 1], writeBuff[MSG_LEN + 1];
	memset(readBuff, '\0', MSG_LEN + 1);
	memset(writeBuff, '\0', MSG_LEN + 1);
	struct pollfd fds[1];
	fds[0].fd = conn;
	fds[0].events = POLLIN | POLLPRI | POLLHUP;
	while(getppid() != 1) { //While the main server is still running
		//Read from client if there is a message to read
		fds[0].revents = 0;
		if(poll(fds, 1, 0) < 0) {
			printf("poll failed\n");
		} else {
			if(fds[0].revents & POLLHUP) { //Check if socket is closed
				return;
			}
			if(fds[0].revents & (POLLIN | POLLPRI)) { //Check if there is data
				if(recv(conn, readBuff, MSG_LEN, 0) < 0) {
					printf("recv failed\n");
				} else {
					strncpy(writeBuff, readBuff, MSG_LEN);
					send(conn, writeBuff, strnlen(writeBuff, MSG_LEN + 1) + 1, 0);
				}
			}
		}
	}
}

/** Main function for server
*/
int main() {
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
	
	//Accept connections
	int conn, process;
	while(1) {
		if((conn = accept(socket_desc, (struct sockaddr *) NULL, NULL)) >= 0) {
			process = fork();
			if(process < 0) {
				printf("Error\n");
			} else if(process == 0) {
				handleConnection(conn);
				//send(conn, "/stop", 6, 0);
				close(conn);
				return 0;
			} else {
				close(conn);
			}
		}
	}
	
	return 0;
}
