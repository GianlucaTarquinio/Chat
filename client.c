#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <arpa/inet.h>
#include <poll.h>

#include "chat.h"

typedef struct threadData {
	int socket;
} ThreadData;

/** Gets input from the user (use in separate thread). Sets threadData->msg
* to a message inputted by the user when threadData->msgAvailable is 0.  Then
* sets threadData->msgAvailable to 1 and waits for it to be 0 again.
* 
* @param threadData ThreadData* containing locations for communication with 
*                   main program
*/
void* getInput(void* threadData) {
	ThreadData* td = (ThreadData*) threadData;
	char msg[MSG_LEN + 1];
	memset(msg, 0, MSG_LEN + 1);
	char *pos;
	char c;
	while(1) {
		fgets(msg, MSG_LEN + 1, stdin);
		if(strlen(msg) > 1) {
			//Remove '\n'
			if((pos = strchr(msg, '\n')) != NULL) {
				*pos = '\0';
			}
			if(strcmp(msg, "/quit") == 0) {
				exit(0);
			}
			//Send to server
			if(send(td->socket, msg, strnlen(msg, MSG_LEN), 0) < 0) {
				printf("Send failed\n"); //Send failed
			}
		}
	}
	exit(1);
}

/** Main function for the program
*/
int main(int argc, char* argv[]) {
	if(argc < 3) {
		printf(BOLD "Usage:" NORMAL " %s <server address>:<port number> <name>\n", argv[0]);
		exit(1);
	}
	char ip[32], *arg, ch;
	memset(ip, 0, 32);
	int i = 0;
	uint16_t port;
	arg = argv[1];
	if(*arg == ':') {
		printf(BOLD "Usage:" NORMAL " %s <server address>:<port number> <name>\n", argv[0]);
		exit(1);
	}
	while((ch = *(arg++)) != ':') {
		if(ch == '\0') {
			printf(BOLD "Usage:" NORMAL " %s <server address>:<port number> <name>\n", argv[0]);
			exit(1);
		}
		ip[i++] = ch;
	}
	ip[i] = '\0';
	port = atoi(arg);
	if(port < 1) {
		printf(BOLD "Usage:" NORMAL " %s <server address>:<port number> <name>\n", argv[0]);
		exit(1);
	}
	int socket_desc = socket(AF_INET, SOCK_STREAM, 0); //Make socket
	if(socket_desc == -1) { //Check for error in socket creation
		printf("Could not create socket\n");
		exit(1);
	}
	//Define server
	struct sockaddr_in server;
	server.sin_addr.s_addr = inet_addr(ip); //This is the ip
	server.sin_family  = AF_INET;
	server.sin_port = htons(port); //This is the port
	memset(server.sin_zero, '\0', sizeof(server.sin_zero));
	//Connect to server
	printf(BOLD "Connecting..." NORMAL "\n");
	if(connect(socket_desc, (struct sockaddr *) &server , sizeof(server)) < 0) {
		printf("Connection error\n"); //Connect failed
		close(socket_desc);
		exit(1);
	}
	
	//Recieve messages from server
	char recvBuf[MSG_BUF_LEN];
	Message incMessage;
	struct pollfd fd;
	fd.fd = socket_desc;
	fd.events = POLLIN | POLLPRI | POLLHUP | POLLERR;
	fd.revents = 0;
	
	//Wait for server code and send name
	char code = 0;
	if(poll(&fd, 1, 10000) < 0) {
		printf("Poll failed\n");
		close(socket_desc);
		exit(1);
	}
	if(fd.revents & (POLLHUP | POLLERR)) {
		printf("Validation failed\n");
		close(socket_desc);
		exit(1);
	}
	if(fd.revents & (POLLIN | POLLPRI)) {
		if(recv(socket_desc, &code, 1, 0) < 0) {
			printf("recv failed\n");
			close(socket_desc);
			exit(1);
		}
	} else {
		printf("Validation failed\n");
		close(socket_desc);
		exit(1);
	}
	if(code != START_CODE) {
		printf("Validation failed\n");
		exit(1);
	}
	if(send(socket_desc, argv[2], strnlen(argv[2], NAME_LEN), 0) < 0) {
		printf("Name send failed\n");
		close(socket_desc);
		exit(1);
	}
	
	//Start accepting input on a separate thread
	pthread_t inputThread;
	ThreadData td;
	td.socket = socket_desc;
	if(pthread_create(&inputThread, NULL, getInput, &td)) {
		printf("Error creating thread\n");
		close(socket_desc);
		exit(1);
	}
	
	printf(BOLD "Connected to %s:%d\nType a message and press enter to send it\nType \"/quit\" to disconnect" NORMAL "\n\n", ip, port);
	int bytesRead;
	while(1) {
		//Read from server if there is a message to read
		fd.revents = 0;
		if(poll(&fd, 1, -1) < 0) {
			printf("poll failed\n");
			close(socket_desc);
			exit(1);
		}
		if(fd.revents & (POLLERR | POLLHUP)) {
			printf(BOLD "Connection lost." NORMAL "\n");
			close(socket_desc);
			exit(0);
		}
		if(fd.revents & (POLLIN | POLLPRI)) { //Check if there is something to read
			bytesRead = recv(socket_desc, recvBuf, MSG_BUF_LEN, 0);
			if(bytesRead < 0) {
				printf("recv failed\n");
			} else if(bytesRead > 0) {
				unserializeMessage(&incMessage, recvBuf);
				switch(incMessage.type) {
					case MSG_NORMAL:
					printf(BOLD "%s:" NORMAL " %s\n", incMessage.name, incMessage.content);
					break;
					
					case MSG_EXIT:
					printf(BOLD "Kicked from server." NORMAL "\n");
					close(socket_desc);
					exit(0);
					break;
					
					case MSG_CONN:
					printf(BOLD "%s connected." NORMAL "\n", incMessage.name);
					break;
					
					case MSG_DCONN:
					printf(BOLD "%s disconnected." NORMAL "\n", incMessage.name);
					break;
					
					case MSG_SERVER:
					printf(BOLD "%s" NORMAL "\n", incMessage.content);
					break;
					
					default:
					//printf("Recieved unrecognized message type: %ld\n, (long) incMessage.type);
					break;
				}
			}
		}
	}
	
	close(socket_desc);
	exit(0);
}
