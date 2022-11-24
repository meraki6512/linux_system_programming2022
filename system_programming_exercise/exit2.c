#include <stdio.h>
#include <unistd.h>

int main(void){

	//_exit: do not buf flush -> print (x)
	printf("Good afternoon?");
	_exit(0);

}
