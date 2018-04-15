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
#include "mode2.h"
#include "mode3.h"
#include "mode4.h"
#include "mode5.h"
#include "dot_font.h"

#define SHM_SIZE 48
#define BUFF_SIZE 64
#define BACK 158
#define VOL_PLUS 115
#define VOL_MINUS 114

#define MODE1 1
#define MODE2 2
#define MODE3 3
#define MODE4 4
#define MODE5 5

#define FND_DEVICE "/dev/fpga_fnd"
#define KEY_DEVICE "/dev/input/event0"
#define SWITCH_DEVICE "/dev/fpga_push_switch"
#define FPGA_BASE_ADDRESS 0x08000000 //fpga_base address
#define LED_ADDR 0x16
#define LCD_DEVICE "/dev/fpga_text_lcd"
#define DOT_DEVICE "/dev/fpga_dot"
#define BUZ_DEVICE "/dev/fpga_buzzer"

#define MAX_BUTTON 9	// for switch
#define MAX_BUFF 32
#define LINE_BUFF 16

#define CLOCK_PER_SEC 100000	// CLOCKS_PER_SEC in <time.h> is 1000000
__inline void delay(clock_t second){
	clock_t start = clock();
	while(clock()-start < CLOCK_PER_SEC*second);
}

int main(){
// prepare variables
	char buf[SHM_SIZE];		// shared memory buffer
	char buf2[SHM_SIZE];
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
	int fd_key, fd_switch, fd_fnd, fd_led, fd_lcd, fd_dot, fd_buz;
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

// Open LCD_DEVICE
	fd_lcd = open(LCD_DEVICE, O_WRONLY);
	if(fd_lcd<0){
		printf("Device open error : %s\n", LCD_DEVICE);
		return -1;
	}

// open DOT_DEVICE
	fd_dot = open(DOT_DEVICE, O_WRONLY);
	if(fd_dot<0){
		printf("Device open error : %s\n", DOT_DEVICE);
		return -1;
	}

// open BUZ_DEVICE
	fd_buz = open(BUZ_DEVICE, O_WRONLY);
	if(fd_buz<0){
		printf("Device open error : %s\n", BUZ_DEVICE);
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
						if(mode > 5) mode = 1;
						break;
					case VOL_MINUS : // subtract mode number
						shmaddr[0] = mode;
						shmaddr[1] = mode_key;
						mode = mode - 1;		// remember mode
						if(mode < 1) mode = 5;
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
				//else				shmaddr[5]='\0';
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
							close(fd_lcd);
							close(fd_buz);
							return 0;
							break;
						case VOL_PLUS : // add mode number
							mode = shmaddr[0];
							mode = mode + 1;
							if(mode>5) mode = 1;
							shmaddr[0] = mode;
							break;
						case VOL_MINUS : // subtract mode number
							mode = shmaddr[0];
							mode = mode - 1;
							if(mode<1) mode = 5;
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
							mode2(shmaddr);
							break;
						case MODE3 :
							delay(2);
							mode3(shmaddr);
							break;
						case MODE4 :
							delay(2);
							mode4(shmaddr);
							break;
						case MODE5 : 
							delay(2);
							mode5(shmaddr);
							break;
						default :	// 이런 경우는 없음
							printf("mode value is wrong. check INPUT PROCESS or MAIN PROCESS\n");
							break;
					}// end of switch(mode)
				semunlock(sem_output);
				main_counter++;
			}
			//return 0;
		}//end of if(pid) - MAIN_PROCESS
// OUTPUT PROCESS - child 2 ----------------------------------------------------------------------------------------------------
		else{	
			int retval;	// check for write
			int str_size;
			while(1){
				shmaddr = (char *)shmat(shmid, NULL, 0);	// 0 = read/write
				semlock(sem_output);
					switch(shmaddr[0]){
// CLOCK MODE : WRITE to FND_DEVICE and LED_DEVICE
						case 1 : // clock mode
							printf("change mode to 1 : clock\n");
						// 버퍼를 이용하여 shm에서 시간만 뽑아내기
							for(i=1;i<5;i++){
								buf[i-1]=shmaddr[i];}
							buf[i]='\0';
							retval = write(fd_fnd, &buf, 4);
							if(retval<0){
								printf("Write Error!\n");
								return -1;
							}
							if(shmaddr[6]==1){	// 수정모드
								delay(1);	// 이게 없으면 SW1이 짝수로만 눌림
								static int flag_blink=0;
								static int blink_value=16; 
								clock_t blink_time;
								if(flag_blink==0){
									blink_time=clock();
									flag_blink = 1;
								}
								clock_t elapsed_time = clock();
								if(elapsed_time - blink_time>CLOCKS_PER_SEC){
									if( (blink_value+16)%32==0 )	*led_addr = 32;
									else							*led_addr = 16;
									flag_blink=0;
									blink_value = blink_value+16;
								}
							}
							else{				// 저장모드 : 1번:2^7
								delay(1);	// 이게 없으면 SW1이 짝수로만 눌림
								*led_addr = 128;
							}
							break;	// END OF CASE 1

// COUNTER MODE : WRITE to FND_DEVICE and LED_DEVICE
						case 2 : // counter mode
							printf("change mode to 2 : counter\n");
						// FND_DEVICE
							for(i=1;i<5;i++){
								buf[i-1]=shmaddr[i];}
							buf[i]='\0';
							delay(1);
							retval = write(fd_fnd, &buf, 4);
							if(retval<0){
								printf("Write Error!\n");
								return -1;
							}
						// LED_DEVICE
							switch(shmaddr[6]){
								case 10 :
									*led_addr = 64;	// 10진수 = 2번 LED
									break;
								case 8 :
									*led_addr = 32;	//  8진수 = 3번 LED
									break;
								case 4 :
									*led_addr = 16; //  4진수 = 4번 LED
									break;
								case 2 :
									*led_addr = 128;//  2진수 = 1번 LED
									break;
								default :
									printf("shmaddr[6] means the base. So check the base in mode2().\n");
									break;
							}
							break;	// END OF CASE 2

// TEXT EDITOR MODE : WRITE to FND_DEVICE, LCD_DEVICE and DOT_DEVICE
						case 3 : // text editor mode
							printf("change mode to 3 : text editor\n");
					// LCD_DEVICE 작성
							//buffer를 이용하여 shmaddr[0]에 적힌 모드부분 제거
							for(i=0;i<=MAX_BUFF;i++) //buffer 청소
								buf[i]=0;
							int len = strlen(shmaddr);
							printf("OUTPUT - strlen(shmaddr) = %d\n",len);
							for(i=0;i<len;i++){
								buf[i] = shmaddr[i+1];
								printf("%c",buf[i]); 	// 문제 해결- MAIN PROCESS에서 mode1 최초진입 아닐때 shmaddr[5]='\0'했길래 수정
							}
							printf("\n");
							//buf[len] = '\0';
							str_size = strlen(buf);
							memset(buf+str_size,' ',MAX_BUFF-str_size);
							write(fd_lcd, &buf, MAX_BUFF);
					// DOT_DEVICE 작성
							if(shmaddr[35]==0){	str_size = sizeof(english); write(fd_dot,english,str_size); }
							else			  { str_size = sizeof(number);  write(fd_dot,number ,str_size); }
					// FND_DEVICE 작성
							for(i=36;i<40;i++){
								buf[i-36]=shmaddr[i];}
							buf[4]='\0';
							retval = write(fd_fnd, &buf, 4);
							if(retval<0){
								printf("Write Error!\n");
								return -1;
							}
							break;	// END OF CASE 3

// DRAW BOARD MODE : WRITE to FND_DEVICE, DOT_DEVICE
						case 4 : // draw board mode
				// shmaddr 변경한게 반영 안되는 문제 발생시 아래쪽 clear shared memory 확인
							printf("change mode to 4 : draw board\n");
				// DOT_DEVICE 작성 
							unsigned char dot_arr[10];	// DOT_DEVICE에 쓸 때 사용
							for(i=0;i<10;i++)	// shmaddr -> dot_arr로 복사
								dot_arr[i]=shmaddr[i+1];
							str_size = sizeof(dot_arr);
							write(fd_dot,dot_arr,str_size);

				// FND_DEVICE 작성
							for(i=15;i<19;i++){
								buf[i-15]=shmaddr[i];}
							buf[4]='\0';
							retval = write(fd_fnd, &buf, 4);
							if(retval<0){
								printf("Write Error!\n");
								return -1;
							}
							break;	// END OF CASE4

// QUIZ GAME MODE : WRITE to DOT_DEVICE, LCD_DEVICE, FND_DEVICE, BUZZER_DEVICE
						case 5 : // quiz game
				// shmaddr 변경한게 반영 안되는 문제 발생시 아래쪽 clear shared memory 확인
							printf("change mode to 5 : quiz mode\n");
				// DOT_DEVICE 작성 
							unsigned char dot_temp[10];	// DOT_DEVICE에 쓸 때 사용
							for(i=0;i<10;i++)	// shmaddr -> dot_arr로 복사
								dot_temp[i]=shmaddr[i+1];
							str_size = sizeof(dot_temp);
							write(fd_dot,dot_temp,str_size);
							if(shmaddr[47]==2) 		delay(10);	// 맞으면 O표시를 1초동안 함 -> X표시는 아래 부저에서 딜레이 처리
				// LCD_DEVICE
							//buffer를 이용하여 shmaddr[0]에 적힌 모드부분 제거
							for(i=0;i<=MAX_BUFF;i++){ //buffer 청소
								buf[i]=0;
								buf2[i]=0;
							}
				/////////////////     shmaddr에서 buf로 옮겨오는 과정   /////////////////////////
						if(shmaddr[48]!=true){					// 수도, 산수 모드
							for(i=0;i<17;i++){
								buf[i] = shmaddr[i+11];
							}
							printf("\n");
							for(i=0;i<17;i++){
								buf2[i] = shmaddr[i+27];
							}
							printf("\n");
						}// end of if(수도, 산수)
						else{									// 속담 모드
							for(i=0;i<33;i++){
								buf[i] = shmaddr[i+11];
							}
						}// end of else(속담)
				///////////////////////////////////////////////////////////////////////////
							// LCD 작성 시작
							char buf_final[33];
							memset(buf_final,0,sizeof(buf_final));
				//////////////////     LCD에 쓰기전 최종 buf에 정리 과정     //////////////////////////////
						if(shmaddr[48]!=true){					// 수도, 산수 모드
							str_size = strlen(buf);
							if(str_size>0){
								strncat(buf_final,buf,str_size);
								memset(buf_final+str_size,' ',LINE_BUFF-str_size);
							}
							str_size = strlen(buf2);
							if(str_size>0){
								strncat(buf_final,buf2,str_size);
								memset(buf_final+LINE_BUFF+str_size,' ',LINE_BUFF-str_size);
							}
						}// end of if(수도, 산수)
						else{									// 속담 모드
							str_size = strlen(buf);
							if(str_size>0){
								strncat(buf_final,buf,str_size);
								memset(buf_final+str_size,' ',MAX_BUFF-str_size);
							}
						}// end of else(속담)
				////////////////////////////////////////////////////////////////////////////
							write(fd_lcd, &buf_final, MAX_BUFF);
						

				// FND_DEVICE 작성
							for(i=43;i<47;i++){
								buf[i-43]=shmaddr[i];}
							buf[4]='\0';
							retval = write(fd_fnd, &buf, 4);
							if(retval<0){
								printf("Write Error!\n");
								return -1;
							}
				// BUZZER_DEVICE 작성
							if(shmaddr[47]==1){
								int retval;
								unsigned char data;

								data=1;
								retval=write(fd_buz,&data,1);
								if(retval<0){
									printf("Write Error!\n");
									return -1;
								}
								delay(10);
								data=0;
								retval=write(fd_buz,&data,1);
								if(retval<0){
									printf("Write Error!\n");
									return -1;
								}
								delay(10);
							}
							break;	// END OF CASE 5

// ERROR! NON EXISTED MODE!
						default :
							printf("Error on calculating mode number\n");
							break;
					}// END OF SWITCH(mode분석 및 MAIN PROCESS에서 넘어온 shmaddr 결과를 보드에 출력)
			// clear shared memory
					for(i=0;i<SHM_SIZE;i++){
						shmaddr[i]='\0';
					}
				semunlock(sem_input);
				
			}// end of while(1) - OUTPUT PROCESS
		}// end of else(pid) - OUTPUT PROCESS
	}// end of fork-else
}// end of main()
