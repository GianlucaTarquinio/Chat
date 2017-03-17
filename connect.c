#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "chat.h"

#define MAX_ADDR_SIZE 50

int getAddressList(char ***addrs) {
	char *line = NULL;
	size_t linecap = MAX_ADDR_SIZE + 1;
	ssize_t len;
	int count = 0, i = 0;
	FILE *f = fopen("save.txt", "r");
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
	printf("Name: ");
	len = getline(&line, &linecap, stdin);
	if(len > 1) {
		line[len - 1] = '\0';
		ret = (char *) malloc(len);
		free(line);
		return line;
	} else {
		free(line);
		return NULL;
	}
}

int getOption(char **addrs, int count) {
	int i;
	printf(BOLD "Servers IPs:" NORMAL "\n");
	for(i = 0; i < count; i++) {
		printf(BOLD "%d: " NORMAL "%s\n", i + 1, addrs[i]);
	}
	printf("Enter the number of the server you want to connect to: ");
	scanf("%d", &i);
	if(i < 1 || i > count) {
		return 0;
	} else {
		return i;
	}
}

int main() {
	char **addrs, *name;
 	int count = getAddressList(&addrs), choice;
	if(count < 0) {
		return 1;
	}
	choice = getOption(addrs, count);
	if(choice == 0) {
		printf("Invalid Option\n");
		return 1;
	}
	name = getName();
	if(!name) {
		printf("Invalid name\n");
		return 1;
	}
	execl("./client", "./client", addrs[choice-1], name, NULL);
	return 0;
}
