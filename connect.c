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

int removeFromSave(int lineIndex, char *path) {
	char *line = NULL, *saveName = "save";
	size_t linecap = 0, written;
	ssize_t len;
	int i = 0, pathLen = strlen(path), nameLen = strlen(saveName);
	char *savePath = (char *) malloc(pathLen + nameLen + 1);
	char *newPath = (char *) malloc(pathLen + nameLen + 2);
	if(!savePath) {
		printf("Error: malloc failed.\n");
		return 1;
	}
	strcpy(savePath, path);
	strcpy(savePath + pathLen, saveName);
	strcpy(newPath, savePath);
	strcpy(newPath + pathLen + nameLen, "~");
	FILE *old = fopen(savePath, "r");
	if(!old) {
		free(savePath);
		free(newPath);
		printf("Error: save file not found.\n");
		return 1;
	}
	FILE *new = fopen(newPath, "w");
	if(!new) {
		free(savePath);
		free(newPath);
		fclose(old);
		printf("Error: couldn't update save file.\n");
		return 1;
	}
	while((len = getline(&line, &linecap, old)) > 0 ) {
		if(*line == ' ') {
			if(i != lineIndex) {
				written = fwrite(line, 1, len, new);
				if(written != len) {
					if(line) {
						free(line);
						line = NULL;
					}
					fclose(old);
					fclose(new);
					unlink(newPath);
					free(savePath);
					free(newPath);
					printf("Error: couldn't update save file.\n");
					return 1;
				}
			}
			i++;
		}
	}
	if(line) {
		free(line);
	}
	fclose(old);
	fclose(new);
	i = rename(newPath, savePath);
	free(savePath);
	free(newPath);
	if(i) {
		printf("Error: couldn't update save file.\n");
		return 1;
	}
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
	if(!f) {
		f = fopen(savePath, "w");
		if(!f) {
			free(savePath);
			printf("Error: couldn't crete save file.\n");
			return -1;
		}
	}
	free(savePath);
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
	char *line;
	line = readline(BOLD "Name: " NORMAL);
	if(!line) {
		return NULL;
	}
	return line;
}

int getOption(SaveData **data, int count, char *prompt) {
	int i;
	char *line;
	printf(BOLD "\nServers:" NORMAL "\n");
	for(i = 0; i < count; i++) {
		printf(BOLD "%d: " NORMAL "%s\n", i + 1, data[i]->name);
	}
	printf("\n");
	line = readline(prompt);
	if(!line) {
		return 0;
	}
	i = atoi(line);
	free(line);
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
		printf("\n" BOLD "J:" NORMAL " Join Server\n");
		printf(BOLD "A:" NORMAL " Add Server\n");
		printf(BOLD "R:" NORMAL " Remove Server\n");
		printf(BOLD "Q:" NORMAL " Quit\n");
		input = readline("\n" BOLD "Enter the option you want (j/a/r/q): " NORMAL);
		if(!input) {
			printf("Error: couldn't read input.\n");
			return 1;
		}
		switch(input[0]) {
			case 'J':
			case 'j':
			choice = getOption(data, count, BOLD "Enter the number of the server you want to connect to: " NORMAL);
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

			case 'A':
			case 'a':
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
					printf("Error: couldn't load saved servers.\n");
					return 1;
				}
			}
			free(addr);
			addr = NULL;
			free(name);
			name = NULL;
			break;

			case 'R':
			case 'r':
			choice = getOption(data, count, BOLD "Enter the number of the server you want to remove: " NORMAL);
			if(choice == 0) {
				printf("Invalid option\n");
				break;
			}
			choice--;
			addr = (char *) malloc(44 + strlen(data[choice]->name) + strlen(data[choice]->addr)); //Yes, I know this isn't an address, but I don't want to make another variable
			sprintf(addr, "Are you sure you want to remove %s (%s)? (y/n) ", data[choice]->name, data[choice]->addr);
			name = readline(addr); //Still don't want to make more variables :^)
			free(addr);
			if(!name) {
				break;
			}
			if((*name == 'y' || *name == 'Y') && !removeFromSave(choice, path)) {
				free(name);
				freeData(&data, count);
				count = loadSaveData(&data, path);
				if(count < 0) {
					printf("Error: counldn't load saved servers.\n");
					return 1;
				}
			} else {
				free(name);
			}
			break;

			case 'Q':
			case 'q':
			exit(0);
			break;

			default:
			printf("Please enter a valid option.\n");
		}
	}
}
