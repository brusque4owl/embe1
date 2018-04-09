#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>

#include "sema.h"
#include "readkey.h"

#define SEM_SIZE 4096

#define BACK 158
#define VOL_PLUS 115
#define VOL_MINUS 114

int main(){
// prepare variables
	char buf[SEM_SIZE];
	int mode=1;	// 초기모드 #1(clock) / fork()후 각 프로세스가 자신의 mode 변수를 가진다.
	int mode_key;			// readkey()함수 리턴값 받는데 사용
	//int i;
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
	/*
	shmaddr = (char *)shmat(shmid, NULL, 0);	// 0 = read/write
	for(i=0;i<SEM_SIZE;i++){
		shmaddr[i] = '0';	// clear shared memory
	}
	*/
// prepare fork
	pid_t pid;

// fork start
	pid = fork();
// INPUT PROCESS - child 1
	if(pid==0){	
		while(1){
			shmaddr = (char *)shmat(shmid, NULL, 0);	// 0 = read/write
			semlock(sem_input);
				/*
				strcpy(shmaddr, "child1 input");
				printf("INPUT PROCESS : %s\n\n", shmaddr);
				*/
				//printf("-------enter readkey()----------\n");
				mode_key = readkey();
				printf("mode_key = %d\n", mode_key);
				switch(mode_key){
					case BACK :	// exit program
						shmaddr[0] = mode;
						shmaddr[1] = mode_key;
						shmaddr[2] = '\0';
						printf("input - mode = %d\t mode_key = %d\n",shmaddr[0], shmaddr[1]);
						shmdt(shmaddr);	// detach shared memory
						// device is closed in readkey() above.
						//exit(0);
						//return 0;
						break;
					case VOL_PLUS : // add mode number
						shmaddr[0] = mode;
						shmaddr[1] = mode_key;
						shmaddr[2] = '\0';
						mode = mode + 1;
						if(mode > 4) mode = 1;
						printf("input - mode = %d\t mode_key = %d\n",shmaddr[0], shmaddr[1]);
						break;
					case VOL_MINUS : // subtract mode number
						shmaddr[0] = mode;
						shmaddr[1] = mode_key;
						shmaddr[2] = '\0';
						mode = mode - 1;
						if(mode < 1) mode = 4;
						printf("input - mode = %d\t mode_key = %d\n",shmaddr[0], shmaddr[1]);
						break;
					default :	// other key
						shmaddr[0] = '\0';	// clear shm
						printf("input(other key) - %d\t%d\n",shmaddr[0], shmaddr[1]);
						break;
				}
			semunlock(sem_main);
			//sleep(3);
		}
	}// end of fork-if
	else{		// parent - MAIN PROCESS
		pid = fork();
// MAIN PROCESS - parent
		if(pid){
			while(1){
				shmaddr = (char *)shmat(shmid, NULL, 0);	// 0 = read/write
				semlock(sem_main);
					/*
					strcpy(buf, shmaddr);	// read from shm
					printf("MAIN PROCESS from buf : %s\n\n", buf);
					strcat(buf, " -- MAIN read from shm");// compute
					strcpy(shmaddr, buf);	// write to shm
					printf("MAIN PROCESS from shm : %s\n\n", shmaddr);
					*/
					//strcpy(buf, shmaddr);
					switch(shmaddr[1]){	// use mode_key
						case BACK : // exit program
							printf("main - mode = %d\t mode_key = %d\n",shmaddr[0], shmaddr[1]);
							shmdt(shmaddr);	// detach shm
							//exit(0);
							return 0;
							break;
						case VOL_PLUS : // add mode number
							mode = shmaddr[0];
							mode = mode + 1;
							if(mode>4) mode = 1;
							shmaddr[0] = mode;
							//shmaddr[1] = '\0';
							printf("main - mode = %d\t mode_key = %d\n",shmaddr[0], shmaddr[1]);
							break;
						case VOL_MINUS : // subtract mode number
							mode = shmaddr[0];
							mode = mode - 1;
							if(mode<1) mode = 4;
							shmaddr[0] = mode;
							//shmaddr[1] = '\0';
							printf("main - mode = %d\t mode_key = %d\n",shmaddr[0], shmaddr[1]);
							break;
						default :	// other key - do nothing
							printf("main(other key) - %d\t%d\n",shmaddr[0], shmaddr[1]);
							break;
					}//end of switch
				semunlock(sem_output);
				//sleep(3);
			}
			//return 0;
		}
// OUTPUT PROCESS - child 2
		else{	
			while(1){
				shmaddr = (char *)shmat(shmid, NULL, 0);	// 0 = read/write
				semlock(sem_output);
					/*
					printf("OUTPUT PROCESS : %s\n\n\n", shmaddr);
					*/
					switch(shmaddr[1]){
						case BACK : // exit program
							printf("output - mode = %d\t mode_key = %d\n",shmaddr[0], shmaddr[1]);
							shmdt(shmaddr);	// detach shm
							//exit(0);
							//return 0;
							break;
						case VOL_PLUS : // change mode + OR
						case VOL_MINUS : // change mode -
							switch(shmaddr[0]){
								case 1 : // clock mode
									printf("change mode to 1 : clock\n");
									break;
								case 2 : // counter mode
									printf("change mode to 2 : counter\n");
									break;
								case 3 : // text editor mode
									printf("change mode to 3 : text editor\n");
									break;
								case 4 : // draw board mode
									printf("change mode to 4 : draw board\n");
									break;
								default :
									printf("Error on calculating mode number\n");
									break;
							}
							printf("output - mode = %d\t mode_key = %d\n\n",shmaddr[0], shmaddr[1]);
							break;
						default : 	// other key - do nothing
							printf("output(other key) - %d\t%d\n\n",shmaddr[0], shmaddr[1]);
							break;
					}//end of switch
				semunlock(sem_input);
				//sleep(3);
			}
		}
	}// end of fork-else
}
