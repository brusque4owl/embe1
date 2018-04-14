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
#include <string.h>

#include "mode5.h"

/*-------- DOT MATRIX에서 주의할점 ----------------------------
|	DOT MATRIX의 경우 y축이 뒤집어져있다.					  |
|	즉 DOWN 스위치를 누르면 Y값이 감소하지 않고 증가한다.	  |
|	반대로 UP 스위치를 누르면 Y값이 감소한다.				  |
|	이러한 사항을 range 처리할 때 기억해두어야한다.			  |
---------------------------------------------------------------*/

#define NO_SWITCH 0
#define SW1    1
#define SW2    2
#define SW3    3
#define SW4    4
#define RESET_COUNTER  5
#define SKIP   6
#define SW7    7
#define SW8    8
#define MODE_CHANGE 9

#define CAPITAL 0 // 수도 문제
#define SAYING  1 // 속담 문제
#define CALC    2 // 산수 문제

#define VOL_PLUS 115
#define VOL_MINUS 114

#define CLOCK_PER_SEC 100000

__inline void update_shm_mode5(char *shmaddr,int dot_matrix[10], /*LCD 변수(상하 2개)*/
								int game_mode, int *question_number, int answer_counter, int enter_mode5){
	// 파라미터 생각해서 조정하기
}

// Global variables - When fpga mode(not game mode) is changed, these must be initialized.
int game_mode = 0;		// Range is 0~2
int question_number[3] = {0,};  // Question number for each game mode. Range is 0~2
int answer_counter = 0;
int enter_mode5 = 0;

int mode5(char *shmaddr){
	int i;
	// Initialize global variables when comes from other mode(mode1 or mode4)
	if(shmaddr[1]==VOL_PLUS || shmaddr[1]==VOL_MINUS){
		game_mode = 0;
		for(i=0;i<3;i++){
			question_number[i] = 0;
		}
		answer_counter = 0;
		enter_mode5 = 0;
	}
// 1. shmaddr으로 들어온 스위치를 분석하여 작업 수행
	switch(shmaddr[2]){
		case SW1 :
			break;
		case SW2 :
			break;
		case SW3 :
			break;
		case SW4 : 
			break;
		case RESET_COUNTER :
			break;
		case SKIP :
			break;
		case MODE_CHANGE :
			game_mode++;
			if(game_mode>2) game_mode=0;
			break;
		default :	// SW7,SW8,NO_SWITCH
			break;
	}// end of switch for analyzing first switch
// 2. Process things - Judge the answer
// 3. Update shared memory
	return 0;
}
