#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void){

	printf("this is the original program\n");
	//exec with arguments list
	execl("./ssu_execl_test_1", "ssu_execl_test_1" "param1", "param2", "param3", (char*)0);
	//because when execl is executed, codes will be overwrited.
	printf("this line should be never get printed\n");

	exit(0);
}
