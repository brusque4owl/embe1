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
#include <sys/mman.h>
#include <time.h>
//---- made header file---//
#include "sema.h"
#include "mode1.h"

#define SHM_SIZE 1024
#define BUFF_SIZE 64
#define BACK 158
#define VOL_PLUS 115
#define VOL_MINUS 114

#define MODE1 1
#define MODE2 2
#define MODE3 3
#define MODE4 4

#define FND_DEVICE "/dev/fpga_fnd"
#define KEY_DEVICE "/dev/input/event0"
#define SWITCH_DEVICE "/dev/fpga_push_switch"
#define FPGA_BASE_ADDRESS 0x08000000 //fpga_base address
#define LED_ADDR 0x16
//#define CLOCK_PER_SEC 100000
#define MAX_BUTTON 9	// for switch
__inline void delay(clock_t second){
	clock_t start = clock();
	while(clock()-start < CLOCKS_PER_SEC*second);
}

int main(){
// prepare variables
	char buf[SHM_SIZE];		// shared memory buffer
	int mode=1;	// 초기모드 #1(clock)
	int i;

// prepare read_key, read_switch
	int mode_key;//mode key : 모드받는변수
	int switch_count = 0;	// 몇개 눌렸는지 체크
	int buff_size;
	struct input_event ev[BUFF_SIZE];
	int rd, value,size=sizeof(struct input_event);// for readkey


	unsigned char push_sw_buff[MAX_BUTTON];
	int pushed_switch[2];		// 몇번 스위치가 눌렸는지 기억

// prepare DEVICES open
	int fd_key, fd_switch, fd_fnd, fd_led;
// open key device, switch device
	if((fd_key = open(KEY_DEVICE, O_RDONLY|O_NONBLOCK))==-1){
		printf("%s is not a valid device\n", KEY_DEVICE);
	}

	fd_switch = open(SWITCH_DEVICE, O_RDWR);
	if(fd_switch<0){
		printf("Device Open Error\n");
		close(fd_key);
		return -1;
	}
// Open FND_DEVICE
	fd_fnd = open(FND_DEVICE, O_RDWR);
	if(fd_fnd<0){
		printf("Device open error : %s\n", FND_DEVICE);
		return -1;
	}
// open LED_DEVICE
	unsigned long *fpga_addr = 0;
	unsigned char *led_addr = 0;
	fd_led = open("/dev/mem", O_RDWR | O_SYNC);
	if(fd_led<0){
		perror("/dev/mem open error");
		exit(1);
	}
	fpga_addr = (unsigned long *)mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd_led, FPGA_BASE_ADDRESS);
	if (fpga_addr == MAP_FAILED){   
		printf("mmap error!\n");
		close(fd_led);
		exit(1);
	}   
	// led data레지스터의 주소값 0x08000016
	led_addr=(unsigned char*)((void*)fpga_addr+LED_ADDR);


// prepare semaphore
	int sem_input,sem_main,sem_output;
	sem_input = initsem(IPC_PRIVATE,1);		// init_val = 1
	sem_main = initsem(IPC_PRIVATE,0);		// init_val = 0
	sem_output = initsem(IPC_PRIVATE,0);	// init_val = 0

// prepare shared memory
	//key_t key;
	int shmid;
	char *shmaddr;
	shmid = shmget(IPC_PRIVATE, SHM_SIZE, IPC_CREAT | 0644);
	if(shmid == -1){
		perror("shmget");
		exit(1);
	}

// prepare fork
	pid_t pid;

// fork start
	pid = fork();
// INPUT PROCESS - child 1 ------------------------------------------------------------------------------------------------
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
						pushed_switch[switch_count]=i+1;	// i+1번째 스위치가 눌린것을 저장(i+1해야 1부터 스위치 시작)
						switch_count++;
					}
				}
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
					default :	// other key - 모드 키 안누르는 경우에도 기존의 mode값은 전송해야함.
						shmaddr[0] = mode;	// 갖고 있던 mode
						shmaddr[1] = '\0';		// 누른키 없음
						break;
				}// end of switch(mode_key)

		// 4. check switch
				if(switch_count==2){
					shmaddr[2] = pushed_switch[0];
					shmaddr[3] = pushed_switch[1];
					i = 4;	// 아래에서 청소하기 위해 기억
				}
				else if(switch_count==1){
					shmaddr[2] = pushed_switch[0];
					i = 3;}
				else{
					i = 2;}
				for(;i<7;i++)
					shmaddr[i] = '\0';

		// 5. print input process	
				printf("input - mode = %d  mode_key = %d  switch1 = %d  switch2 = %d\n",shmaddr[0], shmaddr[1],shmaddr[2],shmaddr[3]);
			semunlock(sem_main);
		}
	}// end of fork-if
	else{		// parent - MAIN PROCESS
		pid = fork();
// MAIN PROCESS - parent ---------------------------------------------------------------------------------------------
		if(pid){
			int main_counter=0;	// main 프로세스 카운터(모드1 최초진입 판별용)
			while(1){
				shmaddr = (char *)shmat(shmid, NULL, 0);	// 0 = read/write
				if(main_counter==0) shmaddr[5]=1;	// 모드1 최초진입시에만 사용
				else				shmaddr[5]='\0';
				semlock(sem_main);
		// 1. check mode key is changed or not
					switch(shmaddr[1]){	// read mode_key
						case BACK : // exit program
							shmdt(shmaddr);	// detach shm
						// close device file descriptor
							close(fd_key);	
							close(fd_switch);
							close(fd_fnd);
							close(fd_led);
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
					}// end of switch(shmaddr[1])
		// 2. Enter the mode (모드 값 계산 후에 해당 모드로 바로 들어가야함.)
						//default :	// other key - 해당 모드로 진입
					switch(mode){
						case MODE1 :
							mode1(shmaddr);
							break;
						case MODE2 :
							break;
						case MODE3 :
							break;
						case MODE4 :
							break;
						default :	// 이런 경우는 없음
							printf("mode value is wrong. check INPUT PROCESS or MAIN PROCESS\n");
							break;
					}// end of switch(mode)
					printf("main - mode = %d\thour[%d%d] minute[%d%d]\n",shmaddr[0],shmaddr[1],shmaddr[2],shmaddr[3],shmaddr[4]);
				semunlock(sem_output);
				main_counter++;
			}
			//return 0;
		}
// OUTPUT PROCESS - child 2 ----------------------------------------------------------------------------------------------------
		else{	
			int retval;	// check for write
			while(1){
				shmaddr = (char *)shmat(shmid, NULL, 0);	// 0 = read/write
				semlock(sem_output);
					switch(shmaddr[0]){
						case 1 : // clock mode
							printf("change mode to 1 : clock\n");
				// CLOCK MODE : WRITE TO FND_DEVICE
						// 버퍼를 이용하여 shm에서 시간만 뽑아내기
							for(i=1;i<5;i++){
								buf[i-1]=shmaddr[i];}
							buf[i]='\0';
							retval = write(fd_fnd, &buf, 4);
							if(retval<0){
								printf("Write Error!\n");
								return -1;
							}
							if(shmaddr[6]==1){	// 수정모드 : 3번:2^5 / 4번:2^4
								delay(1);
								*led_addr = 32;
								delay(1);
								*led_addr = 16;
							}
							else{				// 저장모드 : 1번:2^7
								*led_addr = 128;
							}
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
					}// END OF SWITCH
					printf("output - mode = %d\thour[%d%d] minute[%d%d]\n",shmaddr[0],shmaddr[1],shmaddr[2],shmaddr[3],shmaddr[4]);

			// clear shared memory
					for(i=0;i<SHM_SIZE;i++)
						shmaddr[i]='\0';
				semunlock(sem_input);
				
			}
		}
	}// end of fork-else
}
