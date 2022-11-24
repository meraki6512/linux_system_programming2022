#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/types.h>

#define MAX_RETRIES 8

union ssu_sema_undef{
	struct semid_ds* buf;
	ushort *array;
	int val;
};

int ssu_initsema(key_t key, int sema_nums);

int main(void){

	struct sembuf sema_buf;
	key_t key;
	int sema_id;

	sema_buf.sem_num = 0;
	sema_buf.sem_op = -1;
	sema_buf.sem_flg = SEM_UNDO;

	//get key from "ssu_semademo.c", 'J'
	if ((key = ftok("ssu_semademo.c", 'J'))==-1){
		fprintf(stderr, "ftok error\n");
		exit(1);
	}

	//get semaphore from key and init id to 1
	if ((sema_id = ssu_initsema(key, 1))==-1){
		fprintf(stderr, "initsem error\n");
		exit(1);
	}

	//Lock
	printf("Press return to lock: ");
	getchar();
	printf("Trying to lock... \n");

	if (semop(sema_id, &sema_buf, 1)==-1){
		fprintf(stderr," semop error\n");
		exit(1);
	}

	printf("Locked.\n");

	//Unlock
	printf("Press return to unlock: ");
	getchar();
	sema_buf.sem_op = 1;

	if (semop(sema_id, &sema_buf, 1)==-1){
		fprintf(stderr," semop error\n");
		exit(1);
	}

	printf("Unlocked.\n");
	exit(0);

}

int ssu_initsema(key_t key, int sema_nums){

	union ssu_sema_undef sema;
	struct semid_ds buf;
	struct sembuf sema_buf;
	int i;
	int sema_id;

	sema_id = semget(key, sema_nums, IPC_CREAT|IPC_EXCL|0666);

	if (sema_id>=0){ //semget successed
		sema_buf.sem_op = 1;
		sema_buf.sem_flg = 0;
		sema.val = 1;
		printf("press return\n");
		getchar();

		//P & V
		for (sema_buf.sem_num = 0; sema_buf.sem_num< sema_nums; sema_buf.sem_num++){
			if (semop(sema_id, &sema_buf, 1)==-1){
				int err_num = errno;
				semctl(sema_id, 0, IPC_RMID);
				errno = err_num;
				return -1;
			}
		}
	}
	else if (errno == EEXIST){ //semget error - repeat waiting
		int ready = 0;
		sema_id = semget(key, sema_nums, 0);
		
		if (sema_id < 0)
			return sema_id;

		sema.buf = &buf;

		for (i=0; i<MAX_RETRIES && !ready; i++){
			semctl(sema_id, sema_nums-1, IPC_STAT, sema);

			if (sema.buf -> sem_otime != 0)
				ready = 1;
			else 
				sleep(1);
		}

		if (!ready){
			errno = ETIME;
			return -1;
		}
	}

	return sema_id;
}
