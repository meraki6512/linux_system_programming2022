#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

void ssu_timestamp(char* str);

int main(void){
	
	unsigned int ret;

	//get current time with time() and print with ctime()
	ssu_timestamp("before sleep()"); 
	//do sleep()
	//ret: if 10 sec have passed 0, else left sec
	ret = sleep(10);
	ssu_timestamp("after sleep()");

	printf("sleep() returned %d\n", ret);
	exit(0);

}

void ssu_timestamp(char* str){
	time_t time_val;
	time(&time_val);
	printf("%s the time is %s\n", str, ctime(&time_val));
}
