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

#include "mode4.h"

/*-------- DOT MATRIX에서 주의할점 ----------------------------
|	DOT MATRIX의 경우 y축이 뒤집어져있다.					  |
|	즉 DOWN 스위치를 누르면 Y값이 감소하지 않고 증가한다.	  |
|	반대로 UP 스위치를 누르면 Y값이 감소한다.				  |
|	이러한 사항을 range 처리할 때 기억해두어야한다.			  |
---------------------------------------------------------------*/

#define NO_SWITCH 0
#define SW1 1
#define SW2 2
#define SW3 3
#define SW4 4
#define SW5 5
#define SW6 6
#define SW7 7
#define SW8 8
#define SW9 9

#define RESET 	SW1
#define CURSOR 	SW3
#define SELECT 	SW5
#define CLEAR 	SW7
#define REVERSE SW9

#define UP		SW2
#define DOWN	SW8
#define LEFT	SW4
#define RIGHT	SW6

#define VOL_PLUS 115
#define VOL_MINUS 114

int power(int exp){
	int i,result=1;
	for(i=0;i<exp;i++)
		result = result*2;
	return result;
}

__inline void update_shm_mode4(char *shmaddr,int dot_matrix[10],bool cursor_flag, 
								int cursor_x, int cursor_y, bool cursor_marked, 
								int fnd_counter, int enter_mode4){
	int i;
	//1. dot_matrix 업데이트
	for(i=0;i<10;i++){
		shmaddr[i+1]=dot_matrix[i];
	}
	//2. cursor_flag 업데이트
	if(cursor_flag==1)	shmaddr[11]=1;
	else				shmaddr[11]=0;
	//3. 커서의 좌표 업데이트
	shmaddr[12] = cursor_x;
	shmaddr[13] = cursor_y;
	//4. 커서의 마킹 여부 업데이트
	if(cursor_marked==0) shmaddr[14]=0;
	else				 shmaddr[14]=1;
	//5. FND값 업데이트 shmaddr[15][16][17][18] 네자리
	i=0;
	if(fnd_counter>9999) fnd_counter=0; // 9999를 넘어가면 0으로 초기화
	do{
		shmaddr[18-i]=fnd_counter%10;
		fnd_counter = fnd_counter/10;
		i++;
	}while(fnd_counter!=0);
	//여기 도착하면 i는 counter의 자리수와 같아짐
	switch(i){
		case 1 :	// 3자리 남음
			shmaddr[17]=0;
		case 2 : 	// 2자리 남음
			shmaddr[16]=0;
		case 3 :	// 1자리 남음
			shmaddr[15]=0;
			break;
		default :	// i==4이면 이미 자리수 모두 사용
			break;
	}
	//6. mode 최초 진입 여부 업데이트
	shmaddr[19] = enter_mode4;
}

int mode4(char *shmaddr){
	static bool cursor_flag = true;	// 커서 깜빡임 여부. 초기상태 : 깜빡임
	static int  cursor_x = 0, cursor_y = 0;
	static bool cursor_marked = 0;
	static int  fnd_counter = 0;
	static int  enter_mode4 = 0;	// blink_counter와 연동됨
	static bool matrix[7][10] = {
		{0,0,0,0,0,0,0,0,0,0},
		{0,0,0,0,0,0,0,0,0,0},
		{0,0,0,0,0,0,0,0,0,0},
		{0,0,0,0,0,0,0,0,0,0},
		{0,0,0,0,0,0,0,0,0,0},
		{0,0,0,0,0,0,0,0,0,0},
		{0,0,0,0,0,0,0,0,0,0}
	};
	int dot_matrix[10] ={0,};	// dot_matrix는 mode4()진입 시마다 matrix[7][10]을 바탕으로 새로 계산
	// 모드 변경을 통해 mode4 진입 시
	update_shm_mode4(shmaddr, dot_matrix, cursor_flag, cursor_x, cursor_y, cursor_marked, fnd_counter, enter_mode4);
	return 0;
}
/*
	static int enter_mode3=0;
	static char input=0; // 모드3오면 input을 NUL로 초기화/ 이후 변경내역 기역
	static char string[MAX_BUFF+1];
	static bool eng_num_flag=false;	// false : english | true : number
	static int previous_sw=NO_SWITCH;
	static int repeater = 0;
	static int push_counter = 0;
	if(push_counter==10000) push_counter=0;
	if(shmaddr[1]==VOL_PLUS || shmaddr[1]==VOL_MINUS){
// 모드3 최초진입시 초기화(모드3진입 카운터, input 문자, string 문자열, 영수 플래그, 도트 매트릭스,이전 스위치,문자반복입력,푸시카운터)
		enter_mode3=0;	
		input=0;
		memset(string,0,sizeof(string));
		eng_num_flag=false;
		shmaddr[35]=0;	// 도트 매트리스에 사용할 플래그(0->eng / 1->num)
		previous_sw = NO_SWITCH;
		repeater = 0;
		push_counter = 0;
		//push_counter = 9990; // test code for 9999->0000
	}
*/
