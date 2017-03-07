#ifndef CHAT_H
#define CHAT_H

#include <sys/time.h>

#define MSG_LEN 9000
#define MAX_CONNECTIONS 80

//message types
#define MSG_NORMAL 0
#define MSG_EXIT 1
#define MSG_CONN 2
#define MSG_DCONN 3

typedef struct message {
	char content[MSG_LEN + 1];
	struct timeval date;
	int senderNum;
	unsigned type;
} Message;

int serializeMessage(Message *m, char *buf);
int unserializeMessage(Message *m, char *buf);

#endif