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

#include "mode3.h"

#define SW1 1
#define SW2 2
#define SW3 3
#define SW4 4
#define SW5 5
#define SW6 6
#define SW7 7
#define SW8 8
#define SW9 9

#define VOL_PLUS 115
#define VOL_MINUS 114

#define MAX_BUFF 32
//#define LINE_BUFF 16

/* MAIN의 결과 shmaddr
<옳은 양식>
-----------------------------
|mode|string~~~~~~~~~~~~~~~~|
-----------------------------
 [0]  [1~32]

string과 shmaddr의 관계
   <string>           <shmaddr>
 1. m
 --------------    --------------
 |m           | -> |           m|
 --------------    --------------
 2. mg
 -------------    --------------
 |mg          | -> |          mg|
 --------------    --------------
 3. mgo
 --------------    --------------
 |mgo         | -> |         mgo|
 --------------    --------------
   <string>           <shmaddr>
*/

__inline void update_shm_mode3(char *shmaddr, char *string){
	// string내용은 index 0~31까지 작성되어 있음?
	// shmaddr에는 index 33부터 거꾸로 작성해줘야함. 빈 공간은 ' '로 공백처리 필요.
	int i;
	int len = (int)strlen(string);
	int start = MAX_BUFF-len+1;	// 32는 shmaddr[32]를 의미, len은 string길이, 시작점으로 위치조정을  위해 1을 더해줌
	for(i=0;i<len;i++){
		// string은 index 0부터 저장되고, shmaddr에는 index 32에 끝 글자가 저장된다.(상단주석참고)
		shmaddr[start+i]=string[i];
	}
	// 빈공간 처리('32-문자열길이'만큼 공백 처리)
	if(len<MAX_BUFF){
		for(i=start-1;i>=1;i--){
			shmaddr[i]=' ';
		}
	}
}

int mode3(char *shmaddr){
	int i;
	char input;	// input character
	// [0]~[32] 총 33글자 받고, [33]='\0'
	static char string[MAX_BUFF+2]={'\0',};	
	static bool eng_num_flag=false;	// false : english | true : number

	//1. 버튼 2개 동시에 눌리는 상황 처리(A. SW2&SW3 : LCD clear / B. SW5&SW6 : change writing mode / C. SW8&SW9 : make a space)
	if(shmaddr[2]==SW2 && shmaddr[3]==SW3){
		// LCD CLEAR
		for(i=1;i<=MAX_BUFF;i++)
			shmaddr[i]=' ';
		return 0;
	}
	else if(shmaddr[2]==SW5 && shmaddr[3]==SW6){
		// ENGLISH <-> NUMBER
		eng_num_flag=!eng_num_flag;	// flag 뒤집기
	}
	else if(shmaddr[2]==SW8 && shmaddr[3]==SW9){
		// MAKE A SPACE
	}
	else{	// 하나의 스위치만 누른 경우	
	}

	//2. 글자 처리
	// 만약 글자하나 받았는데 33글자가 되면, 가장 오래된 맨 처음글자 삭제하고, 한칸씩 당긴 뒤 strcat 수행
	if(eng_num_flag==false){	// 영어 입력
	}
	else{						// 숫자 입력
	}
	strcat(string,&input);
	if((int)strlen(string)>32){
		for(i=0;i<=MAX_BUFF;i++){
			string[i]=string[i+1];//한칸씩 당기기(0~31:글자, 32:널문자| 33은 상관없음)
		}
		//string[33]='\0';
	}
	
	printf("strcat 결과 : %s\n",string);
	//3. 마지막에 update_shm_mode3()수행
	update_shm_mode3(shmaddr, string);
	return 0;
}
