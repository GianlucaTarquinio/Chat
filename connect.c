#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <readline/readline.h>

#include "chat.h"

typedef struct saveData {
	char *name;
	char *addr;
} SaveData;

int addToSave(char *name, char *addr, char *path) {
	FILE *f;
	char *saveName = "save";
	size_t pathLen = strlen(path);
	char *savePath = (char *) malloc(pathLen + strlen(saveName) + 1);
	if(!savePath) {
		printf("Error: malloc failed.\n");
		return 1;
	}
	strcpy(savePath, path);
	strcpy(savePath + pathLen, saveName);
	f = fopen(savePath, "a");
	if(!f) {
		printf("Error: save file not found.\n");
		return 1;
	}
	fprintf(f, " %s%c%s\n", name, '\0', addr);
	fclose(f);
	return 0;
}

int loadSaveData(SaveData ***data, char *path) {
	char *line = NULL, *saveName = "save";
	size_t linecap = 0;
	ssize_t len;
	size_t count = 0, i, space;
	size_t pathLen = strlen(path);
	char *savePath = (char *) malloc(pathLen + strlen(saveName) + 1);
	if(!savePath) {
		printf("Error: malloc failed.\n");
		return -1;
	}
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
	*data = (SaveData **) malloc(count * sizeof(SaveData *));
	if(!(*data)) {
		printf("Error: malloc failed.\n");
		fclose(f);
		return -1;
	}
	for(i = 0; i < count; i++) {
		(*data)[i] = (SaveData *) malloc(sizeof(SaveData));
	}
	rewind(f);
	i = 0;
	while(i < count && (len = getline(&line, &linecap, f)) > 1) {
		space = 1;
		if(*line == ' ') {
			line[--len] = '\0';
			while(space < len - 1 && line[space] != '\0') {
				space++;
			}
			space = strnlen(line + 1, len - 2) + 1;
			if(space >= len - 1) {
				if(line) {
					free(line);
					line = NULL;
				}
				fclose(f);
				printf("Error: invalid save format.\n");
				return -1;
			}
			(*data)[i]->name = (char *) malloc(strlen(line + 1) + 1);
			strcpy((*data)[i]->name, line + 1);
			(*data)[i]->addr = (char *) malloc(strlen(line + space + 1) + 1);
			strcpy((*data)[i]->addr, line + space + 1);
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

int getOption(SaveData **data, int count) {
	int i;
	printf(BOLD "\nServers:" NORMAL "\n");
	for(i = 0; i < count; i++) {
		printf(BOLD "%d: " NORMAL "%s\n", i + 1, data[i]->name);
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

void freeData(SaveData ***data, int count) {
	int i;
	for(i = 0; i < count; i++) {
		free((*data)[i]->name);
		free((*data)[i]->addr);
		free((*data)[i]);
	}
	free(*data);
	*data = NULL;
}

int main(int argc, char *argv[]) {
	char *name, *path, *input, *addr;
	SaveData **data = NULL;
	path = getDirectoryPath(argv[0]);
 	int count = loadSaveData(&data, path), choice;
	if(count < 0) {
		printf("Error: couldn't load saved servers.\n");
		return 1;
	}

	while(1) {
		printf("\n" BOLD "1:" NORMAL " Join Server\n");
		printf(BOLD "2:" NORMAL " Add Server\n");
		printf(BOLD "3:" NORMAL " Remove Server\n");
		printf(BOLD "q:" NORMAL " Quit\n");
		input = readline("\n" BOLD "Enter the option you want: " NORMAL);
		if(!input) {
			printf("Error: couldn't read input.\n");
			return 1;
		}
		switch(input[0]) {
			case '1':
			choice = getOption(data, count);
			if(choice == 0) {
				printf("Invalid option\n");
				break;
			}
			name = getName();
			if(!name) {
				printf("Invalid name\n");
				break;
			}
			size_t pathLen = strlen(path);
			char *clientName = "client";
			char *callPath = (char *) malloc(pathLen + strlen(clientName) + 1);
			strcpy(callPath, path);
			strcpy(callPath + pathLen, clientName);
			execl(callPath, callPath, data[choice-1]->addr, name, NULL);
			return 0;
			break;

			case '2':
			name = readline(BOLD "Enter a name for the server: " NORMAL);
			if(!name) {
				printf("Couldn't read name\n");
				break;
			}
			addr = readline(BOLD "Enter the IP of the server: " NORMAL);
			if(!addr) {
				printf("Couldn't read IP\n");
				break;
			}
			if(!addToSave(name, addr, path)) {
				freeData(&data, count);
				count = loadSaveData(&data, path);
				if(count < 0) {
					printf("Error: couldn't load saves servers.\n");
					return 1;
				}
			}
			free(addr);
			addr = NULL;
			free(name);
			name = NULL;
			break;

			case '3':
			printf("3\n");
			break;

			case 'q' :
			exit(0);
			break;

			default:
			printf("Please enter a valid option.\n");
		}
	}
}
