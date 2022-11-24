#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define STRMAX 10000 
#define ARGMAX 10

int tokenize(char *input, char *argv[]);

int main(void)
{
	while (1) {
		char input[STRMAX];
		char *argv[ARGMAX];

		int argc = 0;
		pid_t pid;

		printf("20220000> ");
		fgets(input, sizeof(input), stdin);
		input[strlen(input) - 1] = '\0';
		
		argc = tokenize(input, argv);
		argv[argc] = (char *)0;
		
		if (argc == 0)
			continue;

		if (!strcmp(argv[0], "exit"))
			break;

		if ((pid = fork()) < 0){
			printf("EEROR: fork error\n");
		}
		else if (pid != 0){
			pid = wait(NULL);
		}
		else if (pid == 0){
			if (!strcmp(argv[0], "fmd5") || !strcmp(argv[0], "fsha1"))
				execv(argv[0], argv);
			else
				execl("./help", "./help", (char *)0);
		}
	}
	
	printf("Prompt End\n");
	
	return 0;
}

int tokenize(char *input, char *argv[])
{
	char *ptr = NULL;
	int argc = 0;
	ptr = strtok(input, " ");

	while (ptr != NULL){
		argv[argc++] = ptr;
		ptr = strtok(NULL, " ");
	}
	return argc;
}