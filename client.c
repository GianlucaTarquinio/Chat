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
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
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
	if(argc < 2) {
		printf("Usage: %s <server address>:<portnumber>\n", argv[0]);
		exit(1);
	}
	char ip[32], *arg, ch;
	memset(ip, 0, 32);
	int i = 0;
	uint16_t port;
	arg = argv[1];
	if(*arg == ':') {
		printf("Usage: %s <server address>:<portnumber>\n", argv[0]);
		exit(1);
	}
	while((ch = *(arg++)) != ':') {
		if(ch == '\0') {
			printf("Usage: %s <server address>:<portnumber>\n", argv[0]);
			exit(1);
		}
		ip[i++] = ch;
	}
	ip[i] = '\0';
	port = atoi(arg);
	if(port < 1) {
		printf("Usage: %s <server address>:<portnumber>\n", argv[0]);
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
	printf("Connecting...\n");
	if(connect(socket_desc, (struct sockaddr *) &server , sizeof(server)) < 0) {
		printf("Connection error\n"); //Connect failed
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
	
	//Recieve a reply from the server
	char recvBuf[14 + NAME_LEN + MSG_LEN];
	Message incMessage;
	struct pollfd fds[1];
	fds[0].fd = socket_desc;
	fds[0].events = POLLIN | POLLPRI | POLLHUP;
	printf("Connected to %s:%d\nType a message and press enter to send it\nType \"/quit\" to disconnect\n\n", ip, port);
	while(1) {
		//Read from server if there is a message to read
		fds[0].revents = 0;
		if(poll(fds, 1, -1) < 0) {
			printf("poll failed\n");
			close(socket_desc);
			pthread_cancel(inputThread);
			exit(1);
		}
		if(fds[0].revents & POLLHUP) {
			printf("Connection lost.\n");
			close(socket_desc);
			pthread_cancel(inputThread);
			exit(0);
		}
		if(fds[0].revents & (POLLIN | POLLPRI)) { //Check if there is something to read
			if(recv(socket_desc, recvBuf, 14 + NAME_LEN + MSG_LEN, 0) < 0) {
				printf("recv failed\n");
			} else {
				unserializeMessage(&incMessage, recvBuf);
				switch(incMessage.type) {
					case MSG_NORMAL:
					printf("%s: %s\n", incMessage.name, incMessage.content);
					break;
					
					case MSG_EXIT:
					close(socket_desc);
					pthread_cancel(inputThread);
					exit(0);
					break;
					
					case MSG_CONN:
					printf("User %d connected.\n", incMessage.senderNum);
					break;
					
					case MSG_DCONN:
					printf("User %d disconnected.\n", incMessage.senderNum);
					break;
					
					default:
					//printf("Recieved unrecognized message type: %ld\n, (long) incMessage.type);
					break;
				}
			}
		}
	}
	
	close(socket_desc);
	pthread_cancel(inputThread);
	exit(0);
}
