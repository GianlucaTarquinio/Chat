#ifndef CHAT_H
#define CHAT_H

#include <sys/time.h>
#include <stdint.h>

#define MAX_CONNECTIONS 80
#define MSG_LEN 255
#define NAME_LEN 30

//message types
#define MSG_NORMAL 0
#define MSG_EXIT 1
#define MSG_CONN 2
#define MSG_DCONN 3

typedef struct message {
	uint32_t type;
	char content[MSG_LEN + 1];
	struct timeval date;
	uint32_t senderNum;
	char name[NAME_LEN + 1];
} Message;

void printBuf(char *buf);
int serializeMessage(Message *m, char *buf);
int unserializeMessage(Message *m, char *buf);

#endif