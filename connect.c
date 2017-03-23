#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "chat.h"

#define MAX_ADDR_SIZE 50

int getAddressList(char ***addrs, char *path) {
	char *line = NULL, *saveName = "save.txt";
	size_t linecap = MAX_ADDR_SIZE + 1;
	ssize_t len;
	size_t count = 0, i;
	size_t pathLen = strlen(path);
	char *savePath = (char *) malloc(pathLen + strlen(saveName) + 1);
	strcpy(savePath, path);
	strcpy(savePath + pathLen, saveName);
	FILE *f = fopen(savePath, "r");
	free(savePath);
	if(!f) {
		printf("Error: save file not found.\n");
		return -1;
	}
	while(getline(&line, &linecap, f) > 0) {
		if(*line == ' ') {
			count++;
		}
	}
	if(line) {
		free(line);
		line = NULL;
	}
	*addrs = (char **) malloc(count);
	for(i = 0; i < count; i++) {
		(*addrs)[i] = (char *) malloc(MAX_ADDR_SIZE);
	}
	if(!*addrs) {
		printf("Error: malloc failed.\n");
		fclose(f);
		return -1;
	}
	rewind(f);
	i = 0;
	while(i < count && (len = getline(&line, &linecap, f)) > 1) {
		if(*line == ' ') {
			line[--len] = '\0';
			strcpy((*addrs)[i], line + 1);
			i++;
		}
	}
	if(line) {
		free(line);
		line = NULL;
	}
	fclose(f);
	return count;
}

char *getName() {
	char *line = NULL, *ret;
	size_t linecap = 0;
	ssize_t len;
	while (getchar() != '\n');
	printf(BOLD "Name: " NORMAL);
	len = getline(&line, &linecap, stdin);
	if(len > 1) {
		line[len - 1] = '\0';
		ret = (char *) malloc(len);
		strncpy(ret, line, len);
		free(line);
		return ret;
	} else {
		free(line);
		return NULL;
	}
}

int getOption(char **addrs, int count) {
	int i;
	printf(BOLD "\nServers IPs:" NORMAL "\n");
	for(i = 0; i < count; i++) {
		printf(BOLD "%d: " NORMAL "%s\n", i + 1, addrs[i]);
	}
	printf(BOLD "\nEnter the number of the server you want to connect to: " NORMAL);
	scanf("%d", &i);
	if(i < 1 || i > count) {
		return 0;
	} else {
		return i;
	}
}

char *getDirectoryPath(char *firstArg) {
	int len = strlen(firstArg), i;
	char *path = (char *) malloc(len + 1);
	strcpy(path, firstArg);
	i = len - 1;
	while(i > 0 && path[i] != '/') {
		i--;
	}
	if(i > 0) {
		i++;
	}
	path[i] = '\0';
	return path;
}

int main(int argc, char *argv[]) {
	char **addrs, *name, *path;
	path = getDirectoryPath(argv[0]);
 	int count = getAddressList(&addrs, path), choice;
	if(count < 0) {
		return 1;
	}
	choice = getOption(addrs, count);
	if(choice == 0) {
		printf("Invalid option\n");
		return 1;
	}
	name = getName();
	if(!name) {
		printf("Invalid name\n");
		return 1;
	}
	
	size_t pathLen = strlen(path);
	char *clientName = "client";
	char *callPath = (char *) malloc(pathLen + strlen(clientName) + 1);
	strcpy(callPath, path);
	strcpy(callPath + pathLen, clientName);
	execl(callPath, callPath, addrs[choice-1], name, NULL);
	return 0;
}
