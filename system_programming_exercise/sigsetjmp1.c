//Code you shouldn't actually use

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <setjmp.h>

void ssu_signal_handler(int signo);

jmp_buf jump_buffer;

int main(void){

	//if SIG_INT signal occurs, exe ssu_signal_handler
	signal(SIGINT, ssu_signal_handler);

	while(1){
		if (setjmp(jump_buffer)==0){ //if longjmp occurs ...
			//pause until Ctrl-c is pressed
			printf("Hit Ctrl-c at anytime ... \n"); 
			pause(); 
		}
	}

	exit(1);
}

void ssu_signal_handler(int signo){
	
	char character;

	signal(signo, SIG_IGN);	//if signal occurs, ignore signal (SIG_INT->SIG_IGN)
	
	//Check if Ctrl-c was pressed
	printf("Did you hit Ctrl-c?\nDo you really want to quit? [y/n]\n");
	character = getchar();
	if (character=='Y'||character=='y') //if true -> terminate
		exit(0);
	else{
		signal(SIGINT, ssu_signal_handler); //if false -> make SIGINT signal and...
		longjmp(jump_buffer, 1); // longjmp to jump_buffer
	}
}



