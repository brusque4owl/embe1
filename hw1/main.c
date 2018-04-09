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
	if(pid){	//parent
		pid = fork();
		if(pid){ //parent - MAIN PROCESS
			while(1){
				//shmaddr = (char *)shmat(shmid, NULL, 0);	// 0 = read/write
				semlock(sem_main);
					//printf("parent : %d\n", pid);
					strcpy(buf, shmaddr);	// read from shm
					printf("MAIN PROCESS : %s\n", buf);
					strcat(buf, " -- MAIN read from shm");// compute
					printf("MAIN PROCESS' buf : %s\n",buf);
					strcpy(shmaddr, buf);	// write to shm
				semunlock(sem_output);
				sleep(3);
			}
			return 0;
		}
		else{		// child 2 - OUTPUT PROCESS
			while(1){
				//shmaddr = (char *)shmat(shmid, NULL, 0);	// 0 = read/write
				semlock(sem_output);
					//printf("child 2 : %d\n", pid);
					//strcpy(buf, shmaddr);
					printf("OUTPUT PROCESS : %s\n\n", shmaddr);
				semunlock(sem_input);
				sleep(3);
			}
		}
	}
	else{			// child 1 - INPUT PROCESS
		while(1){
			//shmaddr = (char *)shmat(shmid, NULL, 0);	// 0 = read/write
			semlock(sem_input);
				//printf("child 1 : %d\n", pid);
				strcpy(shmaddr, "child1 input");
				printf("INPUT PROCESS : %s\n", shmaddr);
			semunlock(sem_main);
			sleep(3);
		}
	}
}
