#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>

#define BUFFER_SIZE 1024

struct ssu_msgbuf{
	char msg_text[BUFFER_SIZE];
	long msg_type;
};

int main(void){

	struct ssu_msgbuf buf;
	key_t key;
	int msg_queueid;

	//get key from "ssu_msgqueue_1A.c", 'B'
	if ((key = ftok("ssu_msgqueue_1A.c", 'B'))==-1){
		fprintf(stderr, "ftok error\n");
		exit(1);
	}

	//get (new) msg queue id from key
	if ((msg_queueid = msgget(key, 0644|IPC_CREAT))==-1){
		fprintf(stderr, "msgget error\n");
		exit(1);
	}

	printf("spock: ready to receive messages, captain.\n");

	while (1){

		//receive message(buf) from msg queue id
		if (msgrcv(msg_queueid, &buf, sizeof(buf.msg_text), 0, 0)==-1){
			fprintf(stderr, "msgrcv: Identifier removed\n");
			exit(1);
		}

		printf("spock: \"%s\"\n", buf.msg_text);
	}

	exit(0);
}
