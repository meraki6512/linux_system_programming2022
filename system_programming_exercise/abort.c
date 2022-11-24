#include <stdio.h>
#include <stdlib.h>

int main(void){
	
	printf("abort terminate the program\n");

	//do abort(): abort terminate
	abort(); 

	//after abort termination, will not be executed
	printf("this line is never reached\n");
	exit(0);
}
