#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

//#define FIFO_NAME "SSU-MAID"
#define FIFO_NAME "ssu_fifofile"

int main(void){
	
	char s[300];
	int num, fd;

	//make FIFO file - name: FIFO_NAME
	mknod(FIFO_NAME, S_IFIFO|0666, 0);

	printf("waiting for writers...\n");
	//fd = open(FIFO_NAME, O_RDWR);
	fd = open(FIFO_NAME, O_RDONLY);
	printf("got a writer\n");

	do {
		if ((num = read(fd, s, 300))==-1) //read from fd(fifo file)
			perror("read");
		else {
			s[num] = '\0';
			printf("tick: read %d bytes: \"%s\"\n", num, s);
		}
	} while (num>0);

	exit(0);
}
