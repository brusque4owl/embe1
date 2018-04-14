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
#define CORRECT 3
#define WRONG	4

#define CAPITAL 0 // 수도 문제
#define CALC    1 // 산수 문제
#define SAYING  2 // 속담 문제

#define VOL_PLUS 115
#define VOL_MINUS 114

#define CLOCK_PER_SEC 100000

__inline void update_shm_mode5(char *shmaddr,int dot_mat_num, /*LCD 변수(상하 2개)*/
								char *problem_msg, char *example_msg, int fnd_counter, unsigned char mode_sign_matrix[][10],
								bool saying, bool show_example){
	/*shmaddr[1~10]  : dot matrix
	  shmaddr[11~42] : LCD text
	  shmaddr[43~46] : FNd counter
	  shmaddr[47]    : Buzzer flag
	 */
	int i;
	// 1. Process dot matrix
	for(i=1;i<=10;i++)
		shmaddr[i] = mode_sign_matrix[dot_mat_num][i-1];
	
	// 2. Process LCD text - 수도,산수 / 속담
	if(saying==false){	// 수도, 산수 문제
		for(i=11;i<=26;i++)
			shmaddr[i] = problem_msg[i-11];
		for(i=27;i<=42;i++)
			shmaddr[i] = example_msg[i-27];
	}
	else{	// 속담 문제
		if(show_example==false){	// 문제 표시 32 글자
			for(i=11;i<=42;i++)
				shmaddr[i] = problem_msg[i-11];
		}
		else{						// 보기 표시 32 글자
			for(i=11;i<=42;i++)
				shmaddr[i] = example_msg[i-11];
		}
	}
	// 3. Process answer counter on FND
	i=0;
	if(fnd_counter>9999) fnd_counter=0; // 9999를 넘어가면 0으로 초기화
	do{
		shmaddr[46-i]=fnd_counter%10;
		fnd_counter = fnd_counter/10;
		i++;
	}while(fnd_counter!=0);
	//여기 도착하면 i는 counter의 자리수와 같아짐
	switch(i){
		case 1 :	// 3자리 남음
			shmaddr[45]=0;
		case 2 : 	// 2자리 남음
			shmaddr[44]=0;
		case 3 :	// 1자리 남음
			shmaddr[43]=0;
			break;
		default :	// i==4이면 이미 자리수 모두 사용
			break;
	}

	// 4. Process Buzzer flag
	if(dot_mat_num==WRONG) 			shmaddr[47] = 1;
	else if(dot_mat_num==CORRECT)	shmaddr[47] = 2;
	else							shmaddr[47] = 0;	// WRONG이나 CORRECT가 아닌 경우

	// 5. Saying mode flag
	if(saying==true) shmaddr[48] = 1;
	else			 shmaddr[48] = 0;
}

// Global variables - When fpga mode(not game mode) is changed, these must be initialized.
int game_mode = 0;		// Range is 0~2
int question_number[3] = {0,};  // Question number for each game mode. Range is 0~2
int answer_counter = 0;
int enter_mode5 = 0;
bool saying = false;			// 속담모드에서 사용(game_mode==2면 saying을 true로 만듦)
bool show_example = false;		// 속담모드에서 사용(false:문제보여줌 / true:보기보여줌)
///////////////////////////////////////////////////////////////////////////////////////////
int mode5(char *shmaddr){

// Array for problem set, example set, solution set
char problem[3][10][33] = {
// mode1
	{/*1*/{"KOREA"},/*2*/{"HUNGARY"},/*3*/{"CHINA"},/*4*/{"CANADA"},/*5*/{"BRAZIL"},/*6*/{"JAPAN"},/*7*/{"ALGERIA"},/*8*/{"AUSTRALIA"},/*9*/{"UK"},/*10*/{"GREECE"}},
// mode2
	{/*1*/{"2+3=?"},/*2*/{"3*9=?"},/*3*/{"1+2*8=?"},/*4*/{"2*2+8=?"},/*5*/{"3*2-1*4=?"},/*6*/{"4/2*3+2=?"},/*7*/{"9*6-8/2=?"},/*8*/{"0+3*0=?"},/*9*/{"10/2*8=?"},/*10*/{"1+2+3+4+5=?"}},
// mode3
	{/*1*/{"Do not judge a ( ) by its cover"},/*2*/{"Too many ( ) spoil the broth"},/*3*/{"Many hands make ( ) work"},/*4*/{"There is no place like ( )"},/*5*/{"Easy ( ), easy go"},/*6*/{"( ) is the best policy"},/*7*/{"( ) makes perfect"},/*8*/{"The more, the ( )"},/*9*/{"( ) before you leap"},/*10*/{"The early bird catches the ( )"}}
};

int solution[3][10] = {	// SW1의 값과 직접 비교하기 때문에 1 또는 2로 판단
// mode1 - Capital
	{1,2,2,1,2,2,1,2,1,2},
// mode2 - Calculation
	{2,1,1,1,1,2,1,2,1,1},
// mode3 - Sayings
	{1,2,2,1,2,2,1,2,1,1}
};

char example[3][10][17] = {	// nul 문자 제외하고 16문자로 맞춤
// mode1 - Capital
	{ /*1*/{"Seoul / Pusan"}, /*2*/{"Oslo / Budapest"}, /*3*/{"Paris / Beijing"}, /*4*/{"Ottawa / Rome"}, /*5*/{"Rio / Brasilia"}, /*6*/{"Kyoto / Tokyo"}, /*7*/{"Algiers / Cairo"}, /*8*/{"Dili / Canberra"}, /*9*/{"London / LA"}, /*10*/{"Paris / Athens"} },
// mode2 - Calculation
	{ /*1*/{"4 / 5"}, /*2*/{"27 / 28"}, /*3*/{"17 / 18"}, /*4*/{"12 / 13"}, /*5*/{"2 / 12"}, /*6*/{"7 / 8"}, /*7*/{"50 / 24"}, /*8*/{"9 / 0"}, /*9*/{"40 / 1"}, /*10*/{"15 / 16"} },
// mode3 - Syaings
	{ /*1*/{"book / cook"}, /*2*/{"books / cooks"}, /*3*/{"heavy / light"}, /*4*/{"home / house"}, /*5*/{"get / come"}, /*6*/{"Be / Honesty"}, /*7*/{"Practice / Get"}, /*8*/{"bitter / better"}, /*9*/{"Look / Watch"}, /*10*/{"worm / warm"} }
};


// Dot matrix values for Game mode & O,X sign
unsigned char mode_sign_matrix[5][10] = {
	{0x0c,0x1c,0x1c,0x0c,0x0c,0x0c,0x0c,0x0c,0x0c,0x1e}, // mode 1
	{0x7e,0x7f,0x03,0x03,0x3f,0x7e,0x60,0x60,0x7f,0x7f}, // mode 2
	{0xfe,0x7f,0x03,0x03,0x7f,0x7f,0x03,0x03,0x7f,0x7e}, // mode 3
	{0x3e,0x7f,0x63,0x63,0x63,0x63,0x63,0x63,0x7f,0x3e}, // sign O
	{0x41,0x41,0x22,0x14,0x08,0x08,0x14,0x22,0x41,0x41}  // sign X
};
/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////
	int i;
	int select;	// 선택한 보기
//0. 모드 진입시 변수 초기화  Initialize global variables when comes from other mode(mode1 or mode4)
	if(shmaddr[1]==VOL_PLUS || shmaddr[1]==VOL_MINUS){
		game_mode = 0;
		for(i=0;i<3;i++){
			question_number[i] = 0;
		}
		answer_counter = 0;
		enter_mode5 = 0;
		saying = false;
		show_example = false;
	}
// 1. shmaddr으로 들어온 스위치를 분석하여 작업 수행
	switch(shmaddr[2]){
		case SW1 : // 보기 1번
			select=1;
			break;
		case SW2 : // 보기 2번
			select=2;
			break;
		case SW3 : // 속담 문제에서 보기 볼때 선택
			select=3;
			show_example = !show_example;
			break;
		case RESET_COUNTER :
			select = 5;
			answer_counter = 0;
			break;
		case SKIP :
			select = 6;
			break;
		case MODE_CHANGE :
			game_mode++;
			if(game_mode>2)  game_mode=0;
			// 속담 모드설정
			if(game_mode==2){
				saying = true;
			}
			else{// 비속담 모드이면 속담 관련 플래그 초기화
				saying = false;
				show_example = false;
			}
			select = 9;
			break;
		default :	// SW4, SW7,SW8,NO_SWITCH  // SW3 속담모드 잠시 보류
			select = 0;	// 아무것도 처리 하지 않음
			break;
	}// end of switch for analyzing first switch
// 2. Process things - Judge the answer by 'select'
// 1) Dot matrix 정하기											-- dot_mat_num 처리
	int dot_mat_num;	// OX표시는 아래 정답처리 부분에서 덮어쓰기로 처리
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

// 2) 문제 및 보기 출제(정답 입력 전까지 그 LCD에 내용을 유지해야함		--  problem_msg 처리
	char problem_msg[17];		// 수도와 산수 문제용
	//char problem_msg_long[33];	// 속담 문제용
	char example_msg[17];
	strcpy(problem_msg, problem[game_mode][question_number[game_mode]]);
	strcpy(example_msg, example[game_mode][question_number[game_mode]]);

// 3) 정답 처리(NO_SWITCH 포함 사용하지 않는 키 걸러내야함)		--  answer_counter 처리
	bool correct;
	if(select == SW1 || select == SW2){	// SW1, SW2에 대해서만 정답처리
		if(select == solution[game_mode][question_number[game_mode]])
			 correct = true;
		else correct = false;
		if(correct){ 
			answer_counter++;
			dot_mat_num = CORRECT;
		}// end of if(정답인 경우)
		else{
			dot_mat_num = WRONG;
		}// end of else(오답인 경우)
	}
	else{	// SW3,SW4, SW5(RESET) SW6(SKIP), SW7, SW8, SW9(CHANGE), NO_SWITCH
		// 다른 키
		// reset,change mode는 위에서 값을 처리했음
		// change를 제외한 나머지에 대해서는 현재 상태 유지하도록하기
		// change에 대해서는 바뀐 모드가 출력될 수 있도록 넘겨주기
	}

// 3. Update shared memory
	update_shm_mode5(shmaddr, dot_mat_num, problem_msg, example_msg, answer_counter,mode_sign_matrix,saying,show_example);
	if(select==1 || select==2 || select==6){ // 보기 1번 또는 2번 선택 -> 다음문제로 넘어가도록 문제번호 업데이트
		question_number[game_mode]++; 
		if(question_number[game_mode]>9) question_number[game_mode] = 0;
		show_example = false; // 속담 모드에서는 이걸 풀어줘야 다음문제에서 바로 문제가 나옴
	}// end of if
	enter_mode5++;
	return 0;
}
