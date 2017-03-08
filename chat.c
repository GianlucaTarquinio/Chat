#include "chat.h"

//returns number of bytes written to buf
//[type, senderNum, dateSeconds, name, '\0',            message,          '\0']
// 0     4          8            12    12+strlen(name)  13+strlen(name)   13+strlen(name)+strlen(message)
//maximum strlen(name) is NAME_LEN, maximum strlen(message) is MSG_LEN
int serializeMessage(Message *m, char *buf) {
	return 0;
}

//returns number of bytes read from buf
//[type, senderNum, dateSeconds, name, '\0',            message,          '\0']
// 0     4          8            12    12+strlen(name)  13+strlen(name)   13+strlen(name)+strlen(message)
//maximum strlen(name) is NAME_LEN, maximum strlen(message) is MSG_LEN
int unserializeMessage(Message *m, char *buf) {
	return 0;
}