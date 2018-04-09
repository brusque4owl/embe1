#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <errno.h>

#include "sema.h"
/*-----------------------------
---USAGE OF SEM---
1. semid = initsem(IPC_PRIVATE)
2. semlock(semid)
3. semunlock(semid)
------------------------------*/
int initsem(key_t semkey, int val){
	union semun semunarg;
	int status = 0, semid;

	semid = semget(semkey, 1, IPC_CREAT | IPC_EXCL | 0600);
	if(semid == -1){
		if(errno == EEXIST)
			semid = semget(semkey, 1, 0);
	}
	else{
		semunarg.val = val;
		status = semctl(semid, 0, SETVAL, semunarg);
	}
	if(semid == -1 || status == -1){
		perror("initsem");
		return (-1);
	}
	return semid;
}

int semlock(int semid){
	struct sembuf buf;

	buf.sem_num = 0;
	buf.sem_op = -1;	// DOWN, P()
	buf.sem_flg = SEM_UNDO;
	if(semop(semid, &buf, 1) == -1){
		perror("semlock failed");
		exit(1);
	}
	return 0;
}

int semunlock(int semid){
	struct sembuf buf;
	buf.sem_num = 0;
	buf.sem_op = 1;		// UP, V()
	buf.sem_flg = SEM_UNDO;
	if(semop(semid, &buf, 1) == -1){
		perror("semunlock failed");
		exit(1);
	}
	return 0;
}
