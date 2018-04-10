#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <linux/input.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <signal.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
//---- made header file---//
#include "sema.h"

#define SEM_SIZE 4096
#define BUFF_SIZE 64
#define BACK 158
#define VOL_PLUS 115
#define VOL_MINUS 114

#define MAX_BUTTON 9	// for switch

int main(){
// prepare variables
	//char buf[SEM_SIZE];		// shared memory buffer
	int mode=1;	// 초기모드 #1(clock)
	int i;

// prepare read_key, read_switch
	int fd_key, fd_switch, mode_key;//mode key : 모드받는변수
	int switch_count = 0;	// 몇개 눌렸는지 체크
	int buff_size;
	struct input_event ev[BUFF_SIZE];
	int rd, value,size=sizeof(struct input_event);// for readkey

	char *key_dev = "/dev/input/event0";
	char *switch_dev = "/dev/fpga_push_switch";

	unsigned char push_sw_buff[MAX_BUTTON];
	//bool flag_2_switch = false;	// 스위치 2개가 동시에 눌렸는지 확인
	int pushed_switch[2];		// 몇번 스위치가 눌렸는지 기억

// open key device, switch device
	if((fd_key = open(key_dev, O_RDONLY|O_NONBLOCK))==-1){
		printf("%s is not a valid device\n", key_dev);
	}

	fd_switch = open(switch_dev, O_RDWR);
	if(fd_switch<0){
		printf("Device Open Error\n");
		close(fd_key);
		return -1;
	}

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

// prepare fork
	pid_t pid;

// fork start
	pid = fork();
// INPUT PROCESS - child 1
	if(pid==0){	
		mode = 1;		// 초기모드 1
		while(1){
			shmaddr = (char *)shmat(shmid, NULL, 0);	// 0 = read/write
			semlock(sem_input);
		// initialize
				mode_key = 0;

		// 1. read key
				if((rd=read(fd_key, ev, size * BUFF_SIZE))>=size){
					value = ev[0].value;
					if(value!=' '&&ev[1].value==1&&ev[1].type==1){
						printf("code%d\n", ev[1].code);
					}
					mode_key = ev[0].code;
					printf("key = %d\n",mode_key);
				}

		// 2. read switch
				switch_count = 0;
				buff_size = sizeof(push_sw_buff);
				read(fd_switch, &push_sw_buff, buff_size);
				for(i=0;i<MAX_BUTTON;i++){
					if(push_sw_buff[i]==1){
						pushed_switch[switch_count]=i;	// i번째 스위치가 눌린것을 저장
						switch_count++;
					}
				}
				/*
				switch(switch_count){
					case 2 :	// 2개 스위치가 동시에 눌린 경우
						flag_2_switch = true;
						break;
					default :
						break;
				}
				*/
				/* //print switches
				if(switch_count>0){
					for(i=0;i<MAX_BUTTON;i++){
						printf("[%d] ",push_sw_buff[i]);
					}
					printf("switch_count = %d\n",switch_count);
				}*/

		// 3. check mode_key
				switch(mode_key){
					case BACK :	// exit program
						shmaddr[0] = mode;
						shmaddr[1] = mode_key;
						shmaddr[2] = '\0';
						shmdt(shmaddr);	// detach shared memory
						break;
					case VOL_PLUS : // add mode number
						shmaddr[0] = mode;
						shmaddr[1] = mode_key;
						mode = mode + 1;		// remember mode
						if(mode > 4) mode = 1;
						break;
					case VOL_MINUS : // subtract mode number
						shmaddr[0] = mode;
						shmaddr[1] = mode_key;
						mode = mode - 1;		// remember mode
						if(mode < 1) mode = 4;
						break;
					default :	// other key
						shmaddr[0] = mode;	// 갖고 있던 mode
						shmaddr[1] = 0;		// 누른키 없음
						//printf("input(other key) - %d\t%d\n",shmaddr[0], shmaddr[1]);
						break;
				}// end of switch(mode_key)

		// 4. check switch
				if(switch_count==2){
					shmaddr[2] = pushed_switch[0];
					shmaddr[3] = pushed_switch[1];
					shmaddr[4] = '\0';
				}
				else if(switch_count==1){
					shmaddr[2] = pushed_switch[0];
					shmaddr[3] = '\0';
				}
				else{
					shmaddr[2] = '\0';
					shmaddr[3] = '\0';
				}

		// 5. print input process	
				printf("input - mode = %d  mode_key = %d  switch1 = %d  switch2 = %d\n",shmaddr[0], shmaddr[1],shmaddr[2],shmaddr[3]);
			semunlock(sem_main);
		}
	}// end of fork-if
	else{		// parent - MAIN PROCESS
		pid = fork();
// MAIN PROCESS - parent
		if(pid){
			while(1){
				shmaddr = (char *)shmat(shmid, NULL, 0);	// 0 = read/write
				semlock(sem_main);
					switch(shmaddr[1]){	// read mode_key
						case BACK : // exit program
							shmdt(shmaddr);	// detach shm
							return 0;
							break;
						case VOL_PLUS : // add mode number
							mode = shmaddr[0];
							mode = mode + 1;
							if(mode>4) mode = 1;
							shmaddr[0] = mode;
							break;
						case VOL_MINUS : // subtract mode number
							mode = shmaddr[0];
							mode = mode - 1;
							if(mode<1) mode = 4;
							shmaddr[0] = mode;
							break;
						default :	// other key - do nothing
							printf("main(other key) - %d\t%d\n",shmaddr[0], shmaddr[1]);
							break;
					}//end of switch
					printf("main - mode = %d\t mode_key = %d\n",shmaddr[0], shmaddr[1]);
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
							shmdt(shmaddr);	// detach shm
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
							break;
						default : 	// other key - do nothing
							printf("output(other key) - %d\t%d\n\n",shmaddr[0], shmaddr[1]);
							break;
					}//end of switch
					printf("output - mode = %d\t mode_key = %d\n\n",shmaddr[0], shmaddr[1]);
				semunlock(sem_input);
			}
		}
	}// end of fork-else
}
