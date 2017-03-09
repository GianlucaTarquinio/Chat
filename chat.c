#include <string.h>
#include <stdio.h>

#include "chat.h"

//returns number of bytes written to buf
//[type, senderNum, dateSeconds, name, '\0',            content,          '\0']
// 0     4          8            12    12+strlen(name)  13+strlen(name)   13+strlen(name)+strlen(message)
//maximum strlen(name) is NAME_LEN, maximum strlen(message) is MSG_LEN
//buffer must have size >= 14+NAME_LEN+MSG_LEN
int serializeMessage(Message *m, char *buf) {
	uint32_t type = htonl(m->type);
	uint32_t senderNum = htonl(m->senderNum);
	uint32_t seconds = htonl((uint32_t) m->date.tv_sec);
	int nameLen = strnlen(m->name, NAME_LEN), messageLen = strnlen(m->content, MSG_LEN);
	
	memcpy(buf, &type, 4);
	memcpy(buf + 4, &senderNum, 4);
	memcpy(buf + 8, &seconds, 4);
	strncpy(buf + 12, m->name, nameLen);
	buf[12 + nameLen] = '\0';
	strncpy(buf + 13 + nameLen, m->content, messageLen);
	buf[13 + nameLen + messageLen] = '\0';
	return 14 + nameLen + messageLen;
}

//returns number of bytes read from buf
//[type, senderNum, dateSeconds, name, '\0',            content,          '\0']
// 0     4          8            12    12+strlen(name)  13+strlen(name)   13+strlen(name)+strlen(message)
//maximum strlen(name) is NAME_LEN, maximum strlen(message) is MSG_LEN
//buffer must have size >= 14+NAME_LEN+MSG_LEN
int unserializeMessage(Message *m, char *buf) {
	uint32_t type, senderNum, seconds;
	int nameLen, messageLen;
	nameLen = strnlen(buf + 12, NAME_LEN);
	messageLen = strnlen(buf + 13 + nameLen, MSG_LEN);
	
	memcpy(&type, buf, 4);
	m->type = ntohl(type);
	memcpy(&senderNum, buf + 4, 4);
	m->senderNum = ntohl(senderNum);
	memcpy(&seconds, buf + 8, 4);
	m->date.tv_sec = (time_t) ntohl(seconds);
	strncpy(m->name, buf + 12, nameLen);
	m->name[nameLen] = '\0';
	strncpy(m->content, buf + 13 + nameLen, messageLen);
	m->content[messageLen] = '\0';
	return 14 + nameLen + messageLen;
}
