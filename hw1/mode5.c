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

#define MODE0	0
#define MODE1	1
#define MODE2	2

#define CAPITAL 0 // 수도 문제
#define SAYING  1 // 속담 문제
#define CALC    2 // 산수 문제

#define VOL_PLUS 115
#define VOL_MINUS 114

#define CLOCK_PER_SEC 100000

__inline void update_shm_mode5(char *shmaddr,int game_mode, int dot_mat_num, /*LCD 변수(상하 2개)*/
								char *problem_msg, int answer_counter){
	// 파라미터 생각해서 조정하기
}

// Global variables - When fpga mode(not game mode) is changed, these must be initialized.
int game_mode = 0;		// Range is 0~2
int question_number[3] = {0,};  // Question number for each game mode. Range is 0~2
int answer_counter = 0;
int enter_mode5 = 0;

int mode5(char *shmaddr){
	int i;
	int select;	// 선택한 보기
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
		case SW1 : // 게임모드 0번 선택, 보기 1번
			if(enter_mode5==0)	game_mode=0;	// 게임모드 0번 선택
			else				select=1;		//     보기 1번 선택
			break;
		case SW2 : // 게임모드 1번 선택, 보기 2번
			if(enter_mode5==0)	game_mode=1;	// 게임모드 1번 선택
			else				select=2;		//     보기 2번 선택
			break;
		//case SW3 : // 게임모드 2번 선택 // 속담 문제에서 보기 볼때 선택
		//	if(enter_mode5==0)  game_mode=2;
		//	else				select=3;
		//	break;
		case RESET_COUNTER :
			select = 5;
			answer_counter = 0;
			break;
		case SKIP :
			select = 6;
			break;
		case MODE_CHANGE :
			game_mode++;
			if(game_mode>2) game_mode=0;
			select = 9;
			break;
		default :	// SW4, SW7,SW8,NO_SWITCH  // SW3 속담모드 잠시 보류
			select = 0;	// 아무것도 처리 하지 않음
			break;
	}// end of switch for analyzing first switch
// 2. Process things - Judge the answer by 'select'
	// 1) Dot matrix 정하기											-- dot_mat_num 처리
	int dot_mat_num;
	switch(game_mode){
		case MODE0 :
			dot_mat_num = MODE0;
			break;
		case MODE1 :
			dot_mat_num = MODE1;
			break;
		case MODE2 :
			dot_mat_num = MODE2;
			break;
	}// end of switch(game_mode)
	// 2) 문제 출제(정답 입력 전까지 그 LCD에 내용을 유지해야함		--  problem_msg 처리
	char problem_msg[33];
	strcpy(problem_msg, problem[game_mode][question_number[game_mode]]);
	// 3) 정답 처리(NO_SWITCH 포함 사용하지 않는 키 걸러내야함)		--  answer_counter 처리
	bool correct;
	if(select == SW1 || select == SW2){	// SW1, SW2에 대해서만 정답처리
		if(select == solution[game_mode][question_number[game_mode]])
			 correct = true;
		else correct = false;
		if(correct){ 
			answer_counter++;
			if(answer_counter>99) answer_counter=0;
		}// end of if(정답인 경우)
	}
	else{	// SW3,SW4, SW5(RESET) SW6(SKIP), SW7, SW8, SW9(CHANGE), NO_SWITCH
		// 다른 키
		// reset,change mode는 위에서 값을 처리했음
		// change를 제외한 나머지에 대해서는 현재 상태 유지하도록하기
		// change에 대해서는 바뀐 모드가 출력될 수 있도록 넘겨주기
	}

// 3. Update shared memory
	update_shm_mode5(shmaddr, game_mode, dot_mat_num, problem_msg, answer_counter);
	if(select==1 || select==2){ // 보기 1번 또는 2번 선택 -> 다음문제로 넘어가도록 문제번호 업데이트
		question_number[game_mode]++; 
		if(question_number[game_mode]>9) question_number[game_mode] = 0;
	}// end of if
	return 0;
}
