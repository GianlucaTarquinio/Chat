#ifndef CHAT_H
#define CHAT_H

#define MSG_LEN 9000
#define MAX_CONNECTIONS 80

//type: 0 - normal message
//      1 - exit message
//      2 - user connected
//      3 - user disconnected
typedef struct message {
	char content[MSG_LEN + 1];
	struct timeval date;
	int senderNum;
	int type;
} Message;

#endif