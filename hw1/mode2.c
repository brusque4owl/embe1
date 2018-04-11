#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <time.h>

#include "mode2.h"

#define SW1 1
#define SW2 2
#define SW3 3
#define SW4 4
#define BASE10	1
#define BASE8	2
#define BASE4 	3
#define BASE2 	4

#define VOL_PLUS 115
#define VOL_MINUS 114

__inline void update_shm_mode2(char *shmaddr, int num, int base){
	char temp[30],rev_temp[30];
	int i=0,count=0,end;
	while(num>0){
		temp[i]=num%base;
		num=num/base;
		i++;
	}
	count=i;
	end=count;
	if(count>0){
		//temp뒤집기
		i=1;
		while(count>0){
			rev_temp[i]=temp[count-1]; // 0~count-1
			count--;
			i++;
		}
		if(end>4){	// rev_temp의 끝에서부터 읽어오기
			shmaddr[1] = rev_temp[end-3];
			shmaddr[2] = rev_temp[end-2];
			shmaddr[3] = rev_temp[end-1];
			shmaddr[4] = rev_temp[end];
		}
		if(end==4){
			shmaddr[1]=rev_temp[1];
			shmaddr[2]=rev_temp[2];
			shmaddr[3]=rev_temp[3];
			shmaddr[4]=rev_temp[4];
			if(base==10) shmaddr[1]=0;
		}
		else if(end==3){
			shmaddr[1]=0;
			shmaddr[2]=rev_temp[1];
			shmaddr[3]=rev_temp[2];
			shmaddr[4]=rev_temp[3];
		}
		else if(end==2){
			shmaddr[1]=0;
			shmaddr[2]=0;
			shmaddr[3]=rev_temp[1];
			shmaddr[4]=rev_temp[2];
		}
		else if(end==1){
			shmaddr[1]=0;
			shmaddr[2]=0;
			shmaddr[3]=0;
			shmaddr[4]=rev_temp[1];
		}
		
	}// end of if(count>0)
	else{		// 0000인 경우
		shmaddr[1]=0;
		shmaddr[2]=0;
		shmaddr[3]=0;
		shmaddr[4]=0;
	}
	shmaddr[5]='\0';	// shmaddr[5]
}

int mode2(char *shmaddr){
	static int num;		// 업데이트된 넘버를 기억 / 초기값은 0000
	if(shmaddr[1]==VOL_PLUS || shmaddr[1]==VOL_MINUS) num=0;
	static int base_mode=BASE10;	// 진수를 기억 / 초기값은 10진수
	int base;
	switch(base_mode){
		case BASE10 :
			base = 10;
			break;
		case BASE8 :
			base = 8;
			break;
		case BASE4 :
			base = 4;
			break;
		case BASE2 :
			base = 2;
			break;
	}
	switch(shmaddr[2]){
		case SW1 :
			base_mode++;
			if(base_mode>BASE2) base_mode=BASE10;	//2진수 다음은 10진수로 돌아감
			switch(base_mode){	// base값 변경
			case BASE10 :
				base = 10;
				break;
			case BASE8 :
				base = 8;
				break;
			case BASE4 :
				base = 4;
				break;
			case BASE2 :
				base = 2;
				break;
			}
			break;
		case SW2 :	// base의 제곱을 더함
			switch(base_mode){
				case BASE10 :
					num = num + 100;
					break;
				case BASE8 :
					num = num + 64;
					break;
				case BASE4 :
					num = num + 16;
					break;
				case BASE2 :
					num = num + 4;
					break;
				default :	// 진수가 이상해짐
					printf("WRONG BASE : check the base variable in mode2() function\n");
					break;
			}
			break; // BREAK OF CASE SW2
		case SW3 :	// base값을 더함
			switch(base_mode){
				case BASE10 :
					num = num + 10;
					break;
				case BASE8 :
					num = num + 8;
					break;
				case BASE4 :
					num = num + 4;
					break;
				case BASE2 :
					num = num + 2;
					break;
				default :	// 진수가 이상해짐
					printf("WRONG BASE : check the base variable in mode2() function\n");
					break;
			}
			break; // BREAK OF CASE SW3
		case SW4 :	//모두 1을 더함
			switch(base_mode){
				case BASE10 :
				case BASE8 :
				case BASE4 :
				case BASE2 :
					num = num + 1;
					break;
				default :	// 진수가 이상해짐
					printf("WRONG BASE : check the base variable in mode2() function\n");
					break;
			}
			break; // BREAK OF CASE SW4
		default :	// 다른 키들은 무시
			break;
	}// END OF SWITCH
	update_shm_mode2(shmaddr, num, base);
	return 0;
}
