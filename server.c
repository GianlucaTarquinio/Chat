#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <poll.h>
#include <sys/time.h>

#include "chat.h"
#include "pqueue.h"

typedef struct connectionData {
	int connection;
	pthread_mutex_t lock;
	unsigned valid:1;
	pthread_t thread;
	uint32_t i;
	char name[NAME_LEN + 1];
} ConnectionData;

ConnectionData connections[MAX_CONNECTIONS];
PQueue *messages;
pthread_mutex_t queueLock;
pthread_cond_t toSend;

int messageCompare(const void *a, const void *b) {
	Message *m1 = (Message *) a;
	Message *m2 = (Message *) b;
	if(m1->date.tv_sec < m2->date.tv_usec) {
		return 1;
	} else if(m1->date.tv_sec > m2->date.tv_sec) {
		return -1;
	}
	if(m1->date.tv_usec < m2->date.tv_usec) {
		return 1;
	} else if(m1->date.tv_usec > m2->date.tv_usec) {
		return -1;
	}
	return 0;
}

void *sendMessages(void *param) {
	Message *m;
	uint32_t i;
	char sendBuf[MSG_BUF_LEN];
	int numBytes;
	while(1) {
		pthread_mutex_lock(&queueLock);
		while(messages != NULL) {
			pthread_mutex_unlock(&queueLock);
			m = (Message *) pqPop(&messages, &queueLock);
			numBytes = serializeMessage(m, sendBuf);
			for(i = 0; i < MAX_CONNECTIONS; i++) {
				if(i != m->senderNum) {
					pthread_mutex_lock(&(connections[i].lock));
					if(connections[i].valid) {
						if(send(connections[i].connection, sendBuf, numBytes, 0) < 0) {
							printf("Send failed\n");
						}
					}
					pthread_mutex_unlock(&(connections[i].lock));
				}
			}
			free(m);
			pthread_mutex_lock(&queueLock);
		}
		pthread_cond_wait(&toSend, &queueLock);
		pthread_mutex_unlock(&queueLock);
	}
}

int addMessage(char *buf, uint32_t sender, uint32_t type) {
	struct timeval t;
	int result;
	gettimeofday(&t, NULL);
	Message *m = (Message *) malloc(sizeof(Message));
	m->senderNum = sender;
	m->type = type;
	strncpy(m->content, buf, MSG_LEN);
	*(m->content + MSG_LEN) = '\0';
	m->date = t;
	strncpy(m->name, connections[sender].name, NAME_LEN);
	*(m->name + NAME_LEN) = '\0';
	result = pqPush(&messages, m, messageCompare, &queueLock);
	pthread_cond_signal(&toSend);
	return result;
}

/** Handle a connection with a client
*
* @param conn file descripter for the socket
*/
void *handleConnection(void *param) {
	ConnectionData *me = (ConnectionData *) param;
	char readBuff[MSG_LEN + 1], writeBuff[MSG_LEN + 1];
	memset(readBuff, '\0', MSG_LEN + 1);
	memset(writeBuff, '\0', MSG_LEN + 1);
	struct pollfd fd;
	fd.fd = me->connection;
	fd.events = POLLIN | POLLPRI | POLLHUP;
	fd.revents = 0;
	int bytesRead;
	
	int code = START_CODE;
	if(send(me->connection, &code, 1, 0) < 0) {
		close(me->connection);
		printf("Failed to send code to user %d\n", me->i);
		pthread_mutex_lock(&(me->lock));
		me->valid = 0;
		pthread_mutex_unlock(&(me->lock));
		return NULL;
	}
	
	if(poll(&fd, 1, 10000) < 0) {
		close(me->connection);
		printf("Could not get name from user %d\n", me->i);
		pthread_mutex_lock(&(me->lock));
		me->valid = 0;
		pthread_mutex_unlock(&(me->lock));
		return NULL;
	}
	if(fd.revents & POLLHUP) {
		printf("Could not get name from user %d\n", me->i);
		pthread_mutex_lock(&(me->lock));
		me->valid = 0;
		pthread_mutex_unlock(&(me->lock));
		return NULL;
	}
	if(fd.revents & (POLLIN | POLLPRI)) {
		bytesRead = recv(me->connection, me->name, NAME_LEN, 0);
		if(bytesRead < 1) {
			close(me->connection);
			printf("Could not get name from user %d\n", me->i);
			pthread_mutex_lock(&(me->lock));
			me->valid = 0;
			pthread_mutex_unlock(&(me->lock));
			return NULL;
		}
		me->name[bytesRead] = '\0';
		addMessage("", me->i, MSG_CONN);
	} else {
		close(me->connection);
		printf("Could not get name from user %d\n", me->i);
		pthread_mutex_lock(&(me->lock));
		me->valid = 0;
		pthread_mutex_unlock(&(me->lock));
		return NULL;
	}
	
	while(1) { 
		//Read from client if there is a message to read
		fd.revents = 0;
		if(poll(&fd, 1, -1) < 0) {
			printf("poll failed\n");
		} else {
			if(fd.revents & POLLHUP) { //Check if socket is closed
				printf("Client disconnected from slot %d\n", me->i);
				addMessage("", me->i, MSG_DCONN);
				close(me->connection);
				pthread_mutex_lock(&(me->lock));
				me->valid = 0;
				pthread_mutex_unlock(&(me->lock));
				return NULL;
			}
			if(fd.revents & (POLLIN | POLLPRI)) { //Check if there is data
				bytesRead = recv(me->connection, readBuff, MSG_LEN, 0);
				if(bytesRead < 0) {
					printf("recv failed\n");
				} else {
					readBuff[bytesRead] = '\0';
					addMessage(readBuff, me->i, MSG_NORMAL);
				}
			}
		}
	}
}

/** Main function for server
*/
int main(int argc, char *argv[]) {
	if(argc < 2) {
		printf("Usage: %s <port number>\n", argv[0]);
		return 1;
	}
	
	//init connection data
	if(pthread_mutex_init(&(queueLock), NULL)) {
		printf("Initialization failed\n");
		return 1;
	}
	uint32_t i;
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
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(atoi(argv[1]));
	memset(server.sin_zero, '\0', sizeof(server.sin_zero));
	
	//Bind server struct to socket
	if(bind(socket_desc, (struct sockaddr *) &server, sizeof(server)) < 0) {
		printf("Bind error\n");
		close(socket_desc);
		return 1; 
	}
	
	//Listen on socket, with up to 7 connection requests queued
	if(listen(socket_desc, 7) == 0) {
	} else {
		printf("Error in listen\n");
		return 1;
	}
	
	if(pthread_cond_init(&(toSend), NULL)) {
		printf("Initialization failed\n");
		return 1;
	}
	
	pthread_t sendThread;
	if(pthread_create(&sendThread, NULL, sendMessages, NULL)) {
		printf("Error creating pthread\n");
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
						connections[i].valid = 0;
						close(conn);
					} else {
						printf("Client connected to slot %d\n", i);
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
