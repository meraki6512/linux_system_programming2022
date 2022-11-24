#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define BUFFER_SIZE 1024

int main(void){

	int length;
	char buf[BUFFER_SIZE];

	length = read(0, buf, BUFFER_SIZE);
	write(1, buf, length);

	exit(0);

}
