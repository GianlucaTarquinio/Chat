#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <poll.h>
#include <sys/time.h>

#include "chat.h"
#include "pqueue.h"

#define NUM_CMDS 9

typedef struct connectionData {
	int connection;
	pthread_mutex_t lock;
	unsigned valid:1;
	pthread_t thread;
	uint32_t i;
	char name[NAME_LEN + 1];
} ConnectionData;

typedef struct command {
	char name[CMD_LEN + 1];
	char usage[CMD_USAGE_LEN + 1];
	int (*execCMD)(char *);
} Command;

int socket_desc;
ConnectionData connections[MAX_CONNECTIONS];
PQueue *messages;
Command commands[NUM_CMDS];
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

int addMessage(char *buf, uint32_t sender, uint32_t type) {
	struct timeval t;
	int result;
	gettimeofday(&t, NULL);
	Message *m = (Message *) malloc(sizeof(Message));
	if(!m) {
		return 1;
	}
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

//add reason parameter
void kick(uint32_t i, char *reason) {
	Message m;
	char sendBuf[MSG_BUF_LEN];
	int numBytes;
	m.type = MSG_EXIT;
	if(!reason) {
		strncpy(m.content, "", MSG_LEN);
	} else {
		strncpy(m.content, reason, MSG_LEN);
	}
	m.content[MSG_LEN] = '\0';
	strncpy(m.name, "", NAME_LEN);
	m.name[NAME_LEN] = '\0';
	numBytes = serializeMessage(&m, sendBuf);
	pthread_mutex_lock(&(connections[i].lock));
	if(connections[i].valid) {
		if(send(connections[i].connection, sendBuf, numBytes, 0) < 0) {
			printf("Send failed\n");
		}
	}
	pthread_mutex_unlock(&(connections[i].lock));
}

//////////////////////////////////////////////////
//////////////START OF COMMANDS///////////////////
//////////////////////////////////////////////////

int cmdHardexit(char *args) {
	uint32_t i;
	for(i = 0; i < MAX_CONNECTIONS; i++) {
		pthread_mutex_lock(&(connections[i].lock));
		if(connections[i].valid) {
			close(connections[i].connection);
		}
	}
	close(socket_desc);
	exit(0);
}

int cmdExit(char *args) {
	uint32_t i;
	for(i = 0; i < MAX_CONNECTIONS; i++) {
		kick(i, "Server shutdown");
	}
	close(socket_desc);
	exit(0);
}

int cmdSay(char *args) {
	if(!args) {
		return -1;
	}
	addMessage(args, MAX_CONNECTIONS, MSG_SERVER);
	return 0;
}

int cmdHardkickall(char *args) {
	uint32_t i;
	for(i = 0; i < MAX_CONNECTIONS; i++) {
		pthread_mutex_lock(&(connections[i].lock));
		if(connections[i].valid) {
			close(connections[i].connection);
			connections[i].valid = 0;
		}
		pthread_mutex_unlock(&(connections[i].lock));
	}
	return 0;
}

int cmdKickall(char *args) {
	uint32_t i;
	for(i = 0; i < MAX_CONNECTIONS; i++) {
		kick(i, args);
	}
	return 0;
}

int cmdHardkick(char *args) {
	if(!args || strncmp(args, "", 2) == 0) {
		return -1;
	}
	int num = atoi(args);
	uint32_t i = (uint32_t) num;
	if(i != 0 || strcmp("0", args) == 0) { //kick by number
		if(i >= 0 && i < MAX_CONNECTIONS) {
			pthread_mutex_lock(&(connections[i].lock));
			if(connections[i].valid) {
				close(connections[i].connection);
				connections[i].valid = 0;
			}
			pthread_mutex_unlock(&(connections[i].lock));
		}
	} else { //kick by name
		for(i = 0; i < MAX_CONNECTIONS; i++) {
			pthread_mutex_lock(&(connections[i].lock));
			if(connections[i].valid) {
				if(strncmp(args, connections[i].name, NAME_LEN) == 0) {
					close(connections[i].connection);
					connections[i].valid = 0;
				}
			}
			pthread_mutex_unlock(&(connections[i].lock));
		}
	}
	return 0;
}

int cmdKick(char *args) {
	if(!args || strncmp(args, "", 2) == 0) {
		return -1;
	}
	char *reason = args;
	while(*reason != ' ' && *reason != '\0') {
		reason++;
	}
	if(*reason == ' ') {
		*reason = '\0';
		reason++;
	}
	if(*reason == '\0') {
		reason = NULL;
	}
	int num = atoi(args);
	uint32_t i = (uint32_t) num;
	if(i != 0 || strcmp("0", args) == 0) { //kick by number
		if(i >= 0 && i < MAX_CONNECTIONS) {
			kick(i, reason);
		}
	} else { //kick by name
		for(i = 0; i < MAX_CONNECTIONS; i++) {
			pthread_mutex_lock(&(connections[i].lock));
			if(connections[i].valid) {
				if(strncmp(args, connections[i].name, NAME_LEN) == 0) {
					pthread_mutex_unlock(&(connections[i].lock));
					kick(i, reason);
				} else {
					pthread_mutex_unlock(&(connections[i].lock));
				}
			} else {
				pthread_mutex_unlock(&(connections[i].lock));
			}
		}
	}
	return 0;
}

int cmdList(char *args) {
	uint32_t i;
	for(i = 0; i < MAX_CONNECTIONS; i++) {
		pthread_mutex_lock(&(connections[i].lock));
		if(connections[i].valid) {
			if(!args || strncmp(args, connections[i].name, NAME_LEN) == 0) {
				printf("%ld: %s\n", (long) connections[i].i, connections[i].name);
			}
		}
		pthread_mutex_unlock(&(connections[i].lock));
	}
	return 0;
}

int cmdHelp(char *args) {
	int i;
	printf("Available commands:\n");
	for(i = 0; i < NUM_CMDS; i++) {
		printf("%s %s\n", commands[i].name, commands[i].usage);
	}
	return 0;
}

//////////////////////////////////////////////////
////////////////END OF COMMANDS///////////////////
//////////////////////////////////////////////////

int parseCommand(char *command) {
	int l = 0, r = NUM_CMDS - 1, m, i = 0, cmp, result;
	char *current = command, c;
	i = 0;
	while(i < CMD_LEN && (c = *current) != '\0' && c != '\n' && c != ' ') {
		current++;
		i++;
	}
	if(*current != '\0') { //there are arguments to the command
		*current = '\0';
		current++;
	} else { //there are no arguments
		current = NULL;
	}
	while(l <= r) {
		m = (l + r) / 2;
		cmp = strncmp(command, commands[m].name, CMD_LEN);
		if(cmp > 0) {
			l = m + 1;
		} else if(cmp < 0) {
			r = m - 1;
		} else {
			result = (commands[m].execCMD)(current);
			if(result == -1) {
				printf("Usage: %s %s\n", command, commands[m].usage);
			}
			return result;
		}
	}
	printf("Command not found\n");
	return -2;
}

void addCommand(char *name, char *usage, int (*execCMD)(char *)) {
	static int cmdCount = 0;
	if(cmdCount < NUM_CMDS) {
		strncpy(commands[cmdCount].name, name, CMD_LEN);
		commands[cmdCount].name[CMD_LEN] = '\0';
		if(usage) {
			strncpy(commands[cmdCount].usage, usage, CMD_USAGE_LEN);
			commands[cmdCount].usage[CMD_USAGE_LEN] = '\0';
		} else {
			commands[cmdCount].usage[0] = '\0';
		}
		commands[cmdCount].execCMD = execCMD;
		cmdCount++;
	}
}

int compareCommands(const void *a, const void *b) {
	return strncmp(((Command *) a)->name, ((Command *) b)->name, CMD_LEN);
}

void sortCommands() {
	qsort(&commands, NUM_CMDS, sizeof(Command), compareCommands);
}

void initCommands() {
	addCommand("hardexit", NULL, cmdHardexit);
	addCommand("say", "<message>", cmdSay);
	addCommand("hardkickall", NULL, cmdHardkickall);
	addCommand("list", "[name]", cmdList);
	addCommand("hardkick", "<name | number>", cmdHardkick);
	addCommand("kick", "<name | number>", cmdKick);
	addCommand("kickall", NULL, cmdKickall);
	addCommand("exit", NULL, cmdExit);
	addCommand("help", NULL, cmdHelp);
		
	sortCommands();
}

void *getInput(void *param) {
	char *command = NULL;
	size_t linecap = 0;
	ssize_t len;
	int result;
	while(1) {
		len = getline(&command, &linecap, stdin);
		if(command != NULL) {
			if(len > 1) {
				command[--len] = '\0';
				parseCommand(command);
			}
			free(command);
			command = NULL;
		}
	}
	return NULL;
}

void *sendMessages(void *param) {
	Message *m;
	uint32_t i;
	int numBytes, dateBufLen = 100;
	char sendBuf[MSG_BUF_LEN], dateBuf[dateBufLen];
	while(1) {
		pthread_mutex_lock(&queueLock);
		while(messages != NULL) {
			pthread_mutex_unlock(&queueLock);
			m = (Message *) pqPop(&messages, &queueLock);
			numBytes = serializeMessage(m, sendBuf);
			if(m->type == MSG_NORMAL) {
				if(getTimeString(&(m->date), dateBuf, dateBufLen)) {
					strncpy(dateBuf, "?", dateBufLen - 1);
					dateBuf[dateBufLen - 1] = '\0';
				}
				printf("[%s] %s(%d): %s\n", dateBuf, m->name, m->senderNum, m->content);
			}
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
			usleep(1000); //trying to send over the same connection too frequently causes errors
			pthread_mutex_lock(&queueLock);
		}
		pthread_cond_wait(&toSend, &queueLock);
		pthread_mutex_unlock(&queueLock);
	}
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
	fd.events = POLLIN | POLLPRI | POLLHUP | POLLERR;
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
	if(fd.revents & (POLLHUP | POLLERR)) {
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
			if(fd.revents & (POLLHUP | POLLERR)) { //Check if socket is closed
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
				} else if(bytesRead > 0) {
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
	
	socket_desc = socket(AF_INET, SOCK_STREAM, 0); //Create the socket
	int t = 1;
	setsockopt(socket_desc,SOL_SOCKET,SO_REUSEADDR,&t,sizeof(int));
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
		close(socket_desc);
		return 1;
	}
	
	if(pthread_cond_init(&(toSend), NULL)) {
		printf("Initialization failed\n");
		close(socket_desc);
		return 1;
	}
	
	initCommands();
	
	pthread_t sendThread;
	if(pthread_create(&sendThread, NULL, sendMessages, NULL)) {
		printf("Error creating pthread\n");
		close(socket_desc);
		return 1;
	}
	
	pthread_t inputThread;
	if(pthread_create(&inputThread, NULL, getInput, NULL)) {
		printf("Error creating pthread\n");
		close(socket_desc);
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
	
	close(socket_desc);
	return 0;
}
