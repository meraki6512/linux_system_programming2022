#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void){

	//fork
	if (fork()==0){ //child process
		//exec with arguments list
		execl("/bin/echo", "echo", "this is ""message one", (char*)0);
		//these two lines will be not executed. (because they will be overwrited.)
		fprintf(stderr, "exec error\n");
		exit(1);
	}

	//fork again
	if (fork()==0){ //another child process
		//exec with arguments list
		execl("/bin/echo", "echo", "this is", "message two", (char*)0);
		//these two lines will be not executed. (because they will be overwrited.)
		fprintf(stderr, "exec error\n");
		exit(1);
	}

	//fork again
	if (fork()==0){ //another child process
		//exec with arguments list
		execl("/bin/echo", "echo", "this is" "message three", (char*)0);
		//these two lines will be not executed. (because they will be overwrited.)
		fprintf(stderr, "exec error\n");
		exit(1);
	}

	printf("Parent program ending\n");
	exit(0);

}
