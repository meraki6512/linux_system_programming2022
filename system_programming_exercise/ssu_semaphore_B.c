#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/types.h>

#if defined(_SEM_SEMUN_UNDEFINED) && _SEM_SEMUN_UNDEFINED

union ssu_sema_undef{
	unsigned short int * array;
	struct semid_ds* buf;
	struct seminfo *_buf;
	int val;
};

#endif


int main(void){

	struct sembuf sema;
	key_t key;
	int sema_id;

	//get key from "ssu_semademo.c", 'J'
	if ((key = ftok("ssu_semademo.c", 'J'))==-1){
		fprintf(stderr, "ftok error\n");
		exit(1);
	}

	//get semaphore id
	if ((sema_id = semget(key, 1, 0))==-1){
		fprintf(stderr, "semget error\n");
		exit(1);
	}

	//remove semaphore id
	if (semctl(sema_id, 0, IPC_RMID, sema)==-1){
		fprintf(stderr,"semctl error\n");
		exit(1);
	}

	exit(0);
}
