#ifndef CHAT_H
#define CHAT_H

#define MSG_LEN 255
#define MAX_CONNECTIONS 80

typedef struct message {
	char content[MSG_LEN + 1];
	struct timeval date;
	int senderNum;
} Message;

#endif