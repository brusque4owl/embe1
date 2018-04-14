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

#define VOL_PLUS 115
#define VOL_MINUS 114

#define MAX_BUFF 32
//#define LINE_BUFF 16

/* MAIN의 결과 shmaddr
<옳은 양식>
----------------------------------------------
|mode|string~~~~~~~~~~~~~~~~|E/N flag|counter|
----------------------------------------------
 [0]  [1~32]                  [35]    [36~39]

*/

__inline void update_shm_mode3(char *shmaddr, char *string, char input, bool flag, int repeater, int counter){
	int i;
	int len = (int)strlen(string);
	// FND 처리용 : shmaddr[36][37][38][39] 네자리
	i=0;
	if(counter>9999) counter=0; // 9999를 넘어가면 0으로 초기화
	do{
		shmaddr[39-i]=counter%10;
		counter = counter/10;
		i++;
	}while(counter!=0);
	//여기 도착하면 i는 counter의 자리수와 같아짐
	switch(i){
		case 1 :	// 3자리 남음
			shmaddr[38]=0;
		case 2 : 	// 2자리 남음
			shmaddr[37]=0;
		case 3 :	// 1자리 남음
			shmaddr[36]=0;
			break;
		default :	// i==4이면 이미 자리수 모두 사용
			break;
	}

	// DOT MATRIX 처리용
	if(flag==false) shmaddr[35]=0;	// ENG
	else			shmaddr[35]=1;  // NUM

//아무 입력이 없을 때
	if(input==0){ 	// 길이 32부터는 대기상태에서 널스트링 오는거 처리X
		for(i=0;i<=len;i++){
			shmaddr[i+1]=string[i];
		}
		return;
	}// end of if 아무입력 없을 때

// 입력이 들어오는 경우
// 글자수가 32이상
	if(len>=32){
			if(repeater==1){// 해당 스위치를 처음 입력 -> 글자 작성
					for(i=0;i<len-1;i++){// shmaddr과 string 모두 업데이트
						string[i]=string[i+1]; // 한칸씩 앞으로 이동
						shmaddr[i+1]=string[i+1];
					}
					string[i]=input;	// i=31인 상황
					shmaddr[i+1]=input;
			}
			else{ // 해당 스위치를 반복 입력 -> 기존 마지막 글자만 변환
					for(i=0;i<len-1;i++){ // 1~31글자 복사하고, 32에 input추가
						shmaddr[i+1]=string[i]; // 이동없이 마지막만 수정
					}
					string[i]=input;	// i=31 | string[31]은 string의 32번째 글자
					shmaddr[i+1]=input;
			}
	}// end of if(len>=32)
// 글자수가 32미만
	else{
		//1. shm에 기존 string 작성
		for(i=0;i<len;i++)
			shmaddr[i+1]=string[i];
		if(repeater==1){ // 해당 스위치 최초 입력 -> 글자 작성
			// i==len인 상황
			//2. shm에 input 작성
			shmaddr[i+1]=input;
			//3. string 업데이트
			string[i]=input;
			string[i+1]='\0';
		}
		else{			 // 해당 스위치 반복 입력 -> 기존 마지막 글자 바꿈
			// i==len인 상황
			//2. shm에 input 작성
			shmaddr[i]=input;
			//3. string 업데이트
			string[i-1]=input;
			string[i]='\0';
		}
	}
}

int mode3(char *shmaddr){
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
	int i;


//1. 버튼 2개 동시에 눌리는 상황 처리(A. SW2&SW3 : LCD clear / B. SW5&SW6 : change writing mode / C. SW8&SW9 : make a space)

	//LCD CLAER 작동 확인
	if(shmaddr[2]==SW2 && shmaddr[3]==SW3){
		for(i=1;i<=MAX_BUFF;i++){ // string, input 청소
			shmaddr[i]=' ';// ' ' 공백 아스키 코드==32
			memset(string,0,sizeof(string));
			input=0;
		}
		update_shm_mode3(shmaddr, string, 0, eng_num_flag,repeater,push_counter);
		previous_sw = NO_SWITCH;
		push_counter=push_counter+2;	// switch 2번 누른 것으로 카운트
		repeater = 0;
		return 0;
	}
	// ENGLISH <-> NUMBER 작동 확인 필요
	else if(shmaddr[2]==SW5 && shmaddr[3]==SW6){
		eng_num_flag=!eng_num_flag;	// flag 뒤집기
		input=0;// input은 청소해야함.
		update_shm_mode3(shmaddr, string, 0, eng_num_flag,repeater,push_counter); // flag만 바꾸고 업데이트후 리턴. 다음 차례부터 바뀐모드로 입력받음
		previous_sw = NO_SWITCH;
		push_counter=push_counter+2;	// switch 2번 누른 것으로 카운트
		repeater = 0;
		return 0;
	}
	// MAKE A SPACE 작동 확인
	else if(shmaddr[2]==SW8 && shmaddr[3]==SW9){
		input = ' '; // 공백==32
		repeater = 1; // 공백도 처음 쓰는 것으로 판단
		update_shm_mode3(shmaddr, string, input, eng_num_flag,repeater,push_counter);
		previous_sw = NO_SWITCH;
		push_counter=push_counter+2;	// switch 2번 누른 것으로 카운트
		repeater = 0;
		return 0;
	}
// 버튼 2개 눌리는 상황은 모두 아래까지 안내려오도록하고 리턴시켰음
//2. 글자 처리 -  하나의 스위치만 누른 경우	
//영어 쓰기
if(eng_num_flag==false){ 
	if(shmaddr[2]==NO_SWITCH){	// 아무것도 안누른 경우
		//모드 최초 진입시 string과 LCD 클리어하고 리턴
		// 모드에 반복 진입시 기존 정보로 shm을 update
		// input만 nul로 막기
		input = 0; // 0 = NUL in ASCII CODE(NULL string)
		// previous_sw와 repeater는 계속 기억하고 있어야함.
	}
	else if(shmaddr[2]==SW1){
		if(previous_sw==shmaddr[2]){
			// input을 다음 문자로 바꿔준다.
			switch(repeater%3){
				case 0 :
					input = '.';
					break;
				case 1 :
					input = 'Q';
					break;
				case 2 :
					input = 'Z';
					break;
			}
			repeater++;	// repeater증가
		}
		else{
			// input에 첫번째 대표문자를 넣어준다.
			previous_sw = shmaddr[2];
			repeater = 1;
			input = '.';
		}
		push_counter++;
	}// end of SW1
	else if(shmaddr[2]==SW2){
		if(previous_sw==shmaddr[2]){
			// input을 다음 문자로 바꿔준다.
			switch(repeater%3){
				case 0 :
					input = 'A';
					break;
				case 1 :
					input = 'B';
					break;
				case 2 :
					input = 'C';
					break;
			}
			repeater++;	// repeater증가
		}
		else{
			// input에 첫번째 대표문자를 넣어준다.
			previous_sw = shmaddr[2];
			repeater = 1;
			input = 'A';
		}
		push_counter++;
	}// end of SW2
	else if(shmaddr[2]==SW3){
		if(previous_sw==shmaddr[2]){
			// input을 다음 문자로 바꿔준다.
			switch(repeater%3){
				case 0 :
					input = 'D';
					break;
				case 1 :
					input = 'E';
					break;
				case 2 :
					input = 'F';
					break;
			}
			repeater++;	// repeater증가
		}
		else{
			// input에 첫번째 대표문자를 넣어준다.
			previous_sw = shmaddr[2];
			repeater = 1;
			input = 'D';
		}
		push_counter++;
	}// end of SW3
	else if(shmaddr[2]==SW4){
		if(previous_sw==shmaddr[2]){
			// input을 다음 문자로 바꿔준다.
			switch(repeater%3){
				case 0 :
					input = 'G';
					break;
				case 1 :
					input = 'H';
					break;
				case 2 :
					input = 'I';
					break;
			}
			repeater++;	// repeater증가
		}
		else{
			// input에 첫번째 대표문자를 넣어준다.
			previous_sw = shmaddr[2];
			repeater = 1;
			input = 'G';
		}
		push_counter++;
	}// end of SW4
	else if(shmaddr[2]==SW5){
		if(previous_sw==shmaddr[2]){
			// input을 다음 문자로 바꿔준다.
			switch(repeater%3){
				case 0 :
					input = 'J';
					break;
				case 1 :
					input = 'K';
					break;
				case 2 :
					input = 'L';
					break;
			}
			repeater++;	// repeater증가
		}
		else{
			// input에 첫번째 대표문자를 넣어준다.
			previous_sw = shmaddr[2];
			repeater = 1;
			input = 'J';
		}
		push_counter++;
	}// end of SW5
	else if(shmaddr[2]==SW6){
		if(previous_sw==shmaddr[2]){
			// input을 다음 문자로 바꿔준다.
			switch(repeater%3){
				case 0 :
					input = 'M';
					break;
				case 1 :
					input = 'N';
					break;
				case 2 :
					input = 'O';
					break;
			}
			repeater++;	// repeater증가
		}
		else{
			// input에 첫번째 대표문자를 넣어준다.
			previous_sw = shmaddr[2];
			repeater = 1;
			input = 'M';
		}
		push_counter++;
	}// end of SW6
	else if(shmaddr[2]==SW7){
		if(previous_sw==shmaddr[2]){
			// input을 다음 문자로 바꿔준다.
			switch(repeater%3){
				case 0 :
					input = 'P';
					break;
				case 1 :
					input = 'R';
					break;
				case 2 :
					input = 'S';
					break;
			}
			repeater++;	// repeater증가
		}
		else{
			// input에 첫번째 대표문자를 넣어준다.
			previous_sw = shmaddr[2];
			repeater = 1;
			input = 'P';
		}
		push_counter++;
	}// end of SW7
	else if(shmaddr[2]==SW8){
		if(previous_sw==shmaddr[2]){
			// input을 다음 문자로 바꿔준다.
			switch(repeater%3){
				case 0 :
					input = 'T';
					break;
				case 1 :
					input = 'U';
					break;
				case 2 :
					input = 'V';
					break;
			}
			repeater++;	// repeater증가
		}
		else{
			// input에 첫번째 대표문자를 넣어준다.
			previous_sw = shmaddr[2];
			repeater = 1;
			input = 'T';
		}
		push_counter++;
	}// end of SW8
	else if(shmaddr[2]==SW9){
		if(previous_sw==shmaddr[2]){
			// input을 다음 문자로 바꿔준다.
			switch(repeater%3){
				case 0 :
					input = 'W';
					break;
				case 1 :
					input = 'X';
					break;
				case 2 :
					input = 'Y';
					break;
			}
			repeater++;	// repeater증가
		}
		else{
			// input에 첫번째 대표문자를 넣어준다.
			previous_sw = shmaddr[2];
			repeater = 1;
			input = 'W';
		}
		push_counter++;
	}// end of SW9
}// 영어 쓰기
else{//숫자 쓰기
	switch(shmaddr[2]){
		case SW1 :
			input = '1';
			push_counter++;
			break;
		case SW2 :
			input = '2';
			push_counter++;
			break;
		case SW3 :
			input = '3';
			push_counter++;
			break;
		case SW4 :
			input = '4';
			push_counter++;
			break;
		case SW5 :
			input = '5';
			push_counter++;
			break;
		case SW6 :
			input = '6';
			push_counter++;
			break;
		case SW7 :
			input = '7';
			push_counter++;
			break;
		case SW8 :
			input = '8';
			push_counter++;
			break;
		case SW9 :
			input = '9';
			push_counter++;
			break;
		default : // NO_SWITCH
			input = 0;
			break;
	}// end of switch(shmaddr[2]) of 숫자 쓰기
	repeater=1;
}// end of else 숫자 쓰기
	update_shm_mode3(shmaddr,string,input,eng_num_flag,repeater,push_counter);
	enter_mode3++;
	return 0;
}//END OF mode3()

