#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>

#include "sema.h"

#define SEM_SIZE 4096
int main(){
	char buf[SEM_SIZE];
// prepare semaphore
	int sem_input,sem_main,sem_output;
	sem_input = initsem(IPC_PRIVATE,1);		// init_val = 1
	sem_main = initsem(IPC_PRIVATE,0);		// init_val = 0
	sem_output = initsem(IPC_PRIVATE,0);	// init_val = 0
// prepare shared memory
	//key_t key;
	int shmid;
	char *shmaddr;
	shmid = shmget(IPC_PRIVATE, SEM_SIZE, IPC_CREAT | 0644);
	if(shmid == -1){
		perror("shmget");
		exit(1);
	}
	shmaddr = (char *)shmat(shmid, NULL, 0);	// 0 = read/write
// prepare fork
	pid_t pid;

// fork start
	pid = fork();
	if(pid==0){	// child 1 - INPUT PROCESS
		while(1){
			//shmaddr = (char *)shmat(shmid, NULL, 0);	// 0 = read/write
			semlock(sem_input);
				/*
				strcpy(shmaddr, "child1 input");
				printf("INPUT PROCESS : %s\n\n", shmaddr);
				*/
			semunlock(sem_main);
			sleep(3);
		}
	}// end of fork-if
	else{		// parent - MAIN PROCESS
		pid = fork();
		if(pid){// parent - MAIN PROCESS
			while(1){
				semlock(sem_main);
					/*
					strcpy(buf, shmaddr);	// read from shm
					printf("MAIN PROCESS from buf : %s\n\n", buf);
					strcat(buf, " -- MAIN read from shm");// compute
					strcpy(shmaddr, buf);	// write to shm
					printf("MAIN PROCESS from shm : %s\n\n", shmaddr);
					*/
				semunlock(sem_output);
				sleep(3);
			}
			return 0;
		}
		else{	// child 2 - OUTPUT PROCESS
			while(1){
				//shmaddr = (char *)shmat(shmid, NULL, 0);	// 0 = read/write
				semlock(sem_output);
					/*
					printf("OUTPUT PROCESS : %s\n\n\n", shmaddr);
					*/
				semunlock(sem_input);
				sleep(3);
			}
		}
	}// end of fork-else
}
