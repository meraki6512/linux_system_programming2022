#include "header.h"

int main(void){

	int argc = 0;
	char input[BUF_MAX];
	char* argv[ARG_MAX];
	pid_t pid;

	//prompt
	while(1){
		
		printf("20201482> ");
		fgets(input, sizeof(input), stdin);
		input[strlen(input)-1]='\0';
		argc = split(input, " ", argv);

		if (argc == 0)
			continue;

		//exit
		if ((argc == 1) && (!strcmp(argv[0], "exit"))){
			printf("Prompt End\n");
			break;
		}

		if ((pid = fork())==0){
			//fmd5
			if (!strcmp(argv[0], "fmd5")){
				if (argc != ARG_MAX){
					printf("ERROR: Arguments error\n");
					continue;
				}
				execl("./fmd5", argv[0], argv[1], argv[2], argv[3], argv[4], (char*)0);
			}

			//fsha1
			else if (!strcmp(argv[0], "fsha1")){
				if (argc != ARG_MAX){
					printf("ERROR: Arguments error\n");
					continue;
				}
				execl("./fsha1", argv[0], argv[1], argv[2], argv[3], argv[4], (char*)0);
			}

			//help
			else
				execl("./help", argv[0], (char*)0);
		}
		else if (pid<0){
			fprintf(stderr, "fork error\n");
			exit(1);
		}

		wait((int*)0); 

	}

	return 0;
}





int split(char* string, char* seperator, char* argv[]){

	int argc = 0;
	char* ptr = NULL;

	ptr = strtok(string, seperator);
	while (ptr!=NULL){
		argv[argc++] = ptr;
		ptr = strtok(NULL, seperator);
	}
	return argc;
}




