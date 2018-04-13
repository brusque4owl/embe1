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

#define RESET 	1
#define CURSOR 	3
#define SELECT 	5
#define CLEAR 	7
#define REVERSE 9

#define UP		2
#define DOWN	8
#define LEFT	4
#define RIGHT	6

#define VOL_PLUS 115
#define VOL_MINUS 114

int power(int exp){
	int i,result=1;
	for(i=0;i<exp;i++)
		result = result*2;
	return result;
}

__inline void update_shm_mode4(char *shmaddr,int dot_matrix[10],bool cursor_blink, 
								int cursor_x, int cursor_y, bool cursor_marked, 
								int fnd_counter, int enter_mode4){
	int i;
	//1. dot_matrix 업데이트
	for(i=0;i<10;i++){
		shmaddr[i+1]=dot_matrix[i];
	}
	//2. cursor_blink 업데이트
	if(cursor_blink==1)	shmaddr[11]=1;
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
	int i,j;
	static bool cursor_blink = true;	// 커서 깜빡임 여부. 초기상태 : 깜빡임
	static int  cursor_x = 0, cursor_y = 0;
	static bool cursor_marked = 0;
	static int  fnd_counter = 0;
	static int  enter_mode4 = 0;	// blink_counter와 연동됨
	static bool point_matrix[10][7] = {
		{0,0,0,0,0,0,0}, // 10행 7열의 점 매트릭스 실물
		{0,0,0,0,0,0,0},
		{0,0,0,0,0,0,0},
		{0,0,0,0,0,0,0},
		{0,0,0,0,0,0,0},
		{0,0,0,0,0,0,0},
		{0,0,0,0,0,0,0},
		{0,0,0,0,0,0,0},
		{0,0,0,0,0,0,0},
		{0,0,0,0,0,0,0}
	};
	if(fnd_counter==10000) fnd_counter=0;
// 모드 변경을 통해 mode4 진입시 초기화(커서 깜빡임, 커서 좌표 및 마킹 여부, fnd카운터, 모드4진입 카운터, 매트릭스)
	if(shmaddr[1]==VOL_PLUS || shmaddr[1]==VOL_MINUS){
		cursor_blink = true;
		cursor_x = 0, cursor_y = 0;
		cursor_marked = 0;
		fnd_counter = 0;
		enter_mode4 = 0;
		for(i=0;i<10;i++)
			for(j=0;j<7;j++)
				point_matrix[i][j]=0;
		// for문 끝
	}
	//dot_matrix는 DOT_DEVICE에 작성할 때 사용
	int dot_matrix[10] ={0,};	// mode4()진입 시마다 point_matrix[10][7]을 바탕으로 새로 계산
// shmaddr으로 들어온 스위치를 분석하여 작업 수행(커서이동, 선택(마킹), 리셋, 깜빡임변경, clear, 반전)
	switch(shmaddr[2]){
	// Special keys
		case RESET :
			// Initialize point_matrix
			for(i=0;i<7;i++)
				for(j=0;j<10;j++)
					point_matrix[i][j]=0;
			// Reset values
			cursor_blink = true;
			cursor_x = 0; cursor_y = 0;
			cursor_marked = 0;
			fnd_counter++;
			break;
		case CURSOR :
			cursor_blink = !cursor_blink; // flag반전. 나머지는 유지
			fnd_counter++;
			break;
		case CLEAR :
			// 그림만 clear하고 나머지 유지
			// Initialize point_matrix
			for(i=0;i<7;i++)
				for(j=0;j<10;j++)
					point_matrix[i][j]=0;
			fnd_counter++;
			break;
		case REVERSE :	// 가장 마지막에 구현하기
			fnd_counter++;
			break;
	// General keys
		case SELECT :
			// 현재 커서의 마킹값을 반전시킨 뒤 매트릭스에 저장
			cursor_marked = !cursor_marked;
			// [cursor_x][cursor_y]가 아님에 주의. point_matrix는 [10][7]이고 각각 y값, x값을 나타낸다.
			point_matrix[cursor_y][cursor_x] = cursor_marked;	
			fnd_counter++;
			break;
		case UP : 	 //SW2
			cursor_y--; if(cursor_y<0) cursor_y=0;
			cursor_marked = 0;	// 위치 바꾸면 마킹 여부 초기화
			fnd_counter++;
			break;
		case DOWN :	 //SW8
			cursor_y++; if(cursor_y>9) cursor_y=9;
			cursor_marked = 0;
			fnd_counter++;
			break;
		case LEFT :	 //SW4
			cursor_x--; if(cursor_x<0) cursor_x=0;
			cursor_marked = 0;
			fnd_counter++;
			break;
		case RIGHT : //SW6
			cursor_x++;	if(cursor_x>6) cursor_x=6;
			cursor_marked = 0;
			fnd_counter++;
			break;
		default :	// NO_SWITCH
			printf("Nothing is pushed\n");
			break;		 
	}// end of switch(shmaddr[2]) 분석
// 위에서 작성된 point_matrix를 바탕으로 dot_matrix 작성
	int result;	// dot_matrix에 들어갈 값을 계산
	for(i=0;i<10;i++){
		result=0;
		for(j=0;j<7;j++){
			if(point_matrix[i][j]==true)
				result = result + power(6-j);
		}// row값 계산 완료  ex) 0111001 = 57
		dot_matrix[i]=result;
	}
// UPDATE SHARED MEMORY
	update_shm_mode4(shmaddr, dot_matrix, cursor_blink, cursor_x, cursor_y, cursor_marked, fnd_counter, enter_mode4);
	enter_mode4++;
	printf("cursor_blink = %d\t cursor_x = %d / cursor_y = %d\ncursor_marked = %d\t enter_mode4 = %d\n",cursor_blink,cursor_x,cursor_y,cursor_marked, enter_mode4);
	return 0;
}
