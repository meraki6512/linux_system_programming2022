#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void){

	char* argv[] = {"ssu_execl_test_1", "param1", "param2", (char*)0};
	printf("this is the original program\n");
	//exec with arguments vector
	execv("./ssu_execl_test_1", argv);
	//because when execv executed normally, codes will be changed...
	printf("%s\n", "this line should be never get printed\n");
	exit(0);
}
