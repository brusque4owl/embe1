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

#include "mode1.h"

#define SW1	1
#define SW2 2
#define SW3	3
#define SW4 4

#define VOL_PLUS 115
#define VOL_MINUS 114

// UPDATE SHARED MEMORY FUNCTION
__inline void update_shm_mode1(char *shmaddr, int hour, int minute, bool flag){
	shmaddr[1] = hour/10;
	shmaddr[2] = hour%10;
	shmaddr[3] = minute/10;
	shmaddr[4] = minute%10;
	shmaddr[5] = '\0';
	if(flag==true) shmaddr[6]=1;
	else		   shmaddr[6]=0;
}
/*
INPUT PROCESS에서 전달해주는 shared memeory 형식
------------------------------
|mode|key|input_sw1|input_sw2|
------------------------------
정상적이면 input_sw2는 ='\0'로 막혀있음
*/
int mode1(char *shmaddr){
	static int hour, minute;				// MAIN PROCESS가 따로 보관하는 시간과 분
	static bool flag_clock_change = false;	// 시간,분을 변경하는지 아닌지 체크하는 플래그
	if(shmaddr[1]==VOL_PLUS || shmaddr[1]==VOL_MINUS)	// mode 변경으로 진입시 수정 플래그 false로 세팅
		flag_clock_change = false;
// variables for get board time
	time_t current_time;
	struct tm *gm_time_string;
	gm_time_string = (struct tm *)malloc(sizeof(struct tm));

// 모드1 최초진입시에만 사용(시간 초기화)
	if(shmaddr[5]==1){  
		current_time = time(NULL);
		if(current_time == ((time_t)-1)){
			(void)fprintf(stderr, "Failure to obtain the current time.\n");
			return -1;
		}
		/* Convert to local time format. */
		gm_time_string = localtime(&current_time);
		if(gm_time_string == NULL){
			(void)fprintf(stderr,"Failure to convert the current time.\n");
			return -1;
		}
		hour = gm_time_string->tm_hour; 	// update hour, minute variables
		minute = gm_time_string->tm_min;
	}

	if(shmaddr[2]==SW1){	// SW1의 경우 flag 바꿔주기
		if(flag_clock_change==false) flag_clock_change=true;
		else						 flag_clock_change=false;
	}
	if(flag_clock_change==true){	//SW1으로 변경모드에 들어감
		switch(shmaddr[2]){
			case SW1 :
				break;
			case SW2 : // 보드 시간으로 reset
				current_time = time(NULL);
				if(current_time == ((time_t)-1)){
					(void)fprintf(stderr, "Failure to obtain the current time.\n");
					return -1;
				}
				/* Convert to local time format. */
				gm_time_string = localtime(&current_time);
				if(gm_time_string == NULL){
					(void)fprintf(stderr,"Failure to convert the current time.\n");
					return -1;
				}
				hour = gm_time_string->tm_hour; 	// update hour, minute variables
				minute = gm_time_string->tm_min;
				break;
			case SW3 : // 시간 증가
				hour++;
				if(hour>23) hour=0;
				break;
			case SW4 : // 분 증가
				minute++;
				if(minute>59){
					hour++;
					if(hour>23) hour=0;
					minute=0;
				}
				break;
			default : 	// 시간 변경 모드로 간 뒤 가만히 있을 때 - 지금까지 변경사항을 넘겨줌
				break;
		}
		update_shm_mode1(shmaddr, hour, minute,flag_clock_change);
	}// END of if(flag_clock_change==true)
	else{ // flag_clock_change==false - 모드 처음 진입과 변경 내역 저장을 제외하고는 아무것도 안함
		switch(shmaddr[2]){
			case SW1 :	// flag가 true에서 false로 바뀌면 저장해야함.
				break;
			case SW2 :	// 수정모드가 아닐 때는 저장된 시간을 출력해준다.
			case SW3 :
			case SW4 :
				break;
			default :	
				if(shmaddr[1]==VOL_PLUS || shmaddr[1]==VOL_MINUS){	// 모드1 진입 시, 보드 시간으로 초기화 
					current_time = time(NULL);
					if(current_time == ((time_t)-1)){
						(void)fprintf(stderr, "Failure to obtain the current time.\n");
						return -1;
					}
					/* Convert to local time format. */
					gm_time_string = localtime(&current_time);
					if(gm_time_string == NULL){
						(void)fprintf(stderr,"Failure to convert the current time.\n");
						return -1;
					}
					hour = gm_time_string->tm_hour; 	// update hour, minute variables
					minute = gm_time_string->tm_min;
					break;
				}
				else{					// 변경 내역 저장 후 가만히 있을 때.
					break;
				}
		}
		update_shm_mode1(shmaddr, hour, minute,flag_clock_change);
	}// END of else(보드 시간으로 초기화)
	return 0;
}
