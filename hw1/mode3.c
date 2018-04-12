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
-----------------------------
|mode|string~~~~~~~~~~~~~~~~|
-----------------------------
 [0]  [1~32]

*/

__inline void update_shm_mode3(char *shmaddr, char *string, char input){
	int i;
	shmaddr[0]=3;	// mode 3
	int len = (int)strlen(string);
	if(len>=32){
		if(input==0){ 	// 길이 32부터는 대기상태에서 널스트링 오는거 처리X
			for(i=0;i<=len;i++){
				shmaddr[i+1]=string[i];
			}
			return;
		}
		for(i=0;i<len-1;i++){// shmaddr과 string 모두 업데이트
			string[i]=string[i+1]; // 한칸씩 앞으로 이동
			shmaddr[i+1]=string[i+1];
		}
		string[i]=input;	// i=31인 상황
		shmaddr[i+1]=input;
	}
	else{
		//1. shm에 string 작성
		for(i=1;i<=len;i++)
			shmaddr[i]=string[i-1];
		//2. shm에 input 작성
		shmaddr[i]=input;
		//3. string 업데이트
		string[i-1]=input;
		string[i]='\0';
	}

	printf("업데이트 전 : %d\n",len);
	printf("스트링 길이 : %d\n",(int)strlen(string));
	printf("    i value : %d\n", i);
	printf("shm    길이 : %d\n",(int)strlen(shmaddr));	// shmaddr[0]=3이 있어서, 스트링 없어도 길이가 1이됨.
	for(i=0;i<(int)strlen(string);i++)
		printf("%c",string[i]);
	printf("\n");
	for(i=0;i<(int)strlen(shmaddr);i++)
		printf("%c",shmaddr[i]);
	printf("\n");
}


int mode3(char *shmaddr){
	static int enter_mode3=0;
	static char input=0; // 모드3오면 input을 NUL로 초기화/ 이후 변경내역 기역
	static char string[MAX_BUFF+1];
	if(shmaddr[1]==VOL_PLUS || shmaddr[1]==VOL_MINUS){
		enter_mode3=0;	// 모드3 최초진입시 초기화(모드3진입 카운터, input 문자, string 문자열) 
		input=0;
		memset(string,0,sizeof(string));
	}
	int i;

	static bool eng_num_flag=false;	// false : english | true : number

//1. 버튼 2개 동시에 눌리는 상황 처리(A. SW2&SW3 : LCD clear / B. SW5&SW6 : change writing mode / C. SW8&SW9 : make a space)

	//LCD CLAER 작동 확인
	if(shmaddr[2]==SW2 && shmaddr[3]==SW3){
		for(i=1;i<=MAX_BUFF;i++){ // string, input 청소
			shmaddr[i]=' ';// ' ' 공백 아스키 코드==32
			memset(string,0,sizeof(string));
			input=0;
		}
		return 0;
	}
	// ENGLISH <-> NUMBER 작동 확인 필요
	else if(shmaddr[2]==SW5 && shmaddr[3]==SW6){
		eng_num_flag=!eng_num_flag;	// flag 뒤집기
		input=0;// input은 청소해야함.
		update_shm_mode3(shmaddr, string, 0); // flag만 바꾸고 업데이트후 리턴. 다음 차례부터 바뀐모드로 입력받음
		return 0;
	}
	// MAKE A SPACE 작동 확인
	else if(shmaddr[2]==SW8 && shmaddr[3]==SW9){
		input = ' '; // 공백==32
		update_shm_mode3(shmaddr, string, input);
		return 0;
	}
// 버튼 2개 눌리는 상황은 모두 아래까지 안내려오도록하고 리턴시켰음
//2. 글자 처리 -  하나의 스위치만 누른 경우	

	if(shmaddr[2]==NO_SWITCH){	// 아무것도 안누른 경우
		//모드 최초 진입시 string과 LCD 클리어하고 리턴
		// 모드에 반복 진입시 기존 정보로 shm을 update
		// input만 nul로 막기
		input = 0; // 0 = NUL in ASCII CODE(NULL string)
	}
	else if(shmaddr[2]==SW1){
		input = '.';
	}
	else if(shmaddr[2]==SW2){
		input = 'A';
	}
	else if(shmaddr[2]==SW3){
		input = 'D';
	}
	else if(shmaddr[2]==SW4){
		input = 'G';
	}
	else if(shmaddr[2]==SW5){
		input = 'J';
	}
	else if(shmaddr[2]==SW6){
		input = 'M';
	}
	else if(shmaddr[2]==SW7){
		input = 'P';
	}
	else if(shmaddr[2]==SW8){
		input = 'T';
	}
	else if(shmaddr[2]==SW9){
		input = 'W';
	}
	update_shm_mode3(shmaddr,string,input);
	enter_mode3++;
	printf("@@@@@enter_mode3 = %d\n",enter_mode3);
	return 0;
}//END OF mode3()
/*
	//1. 버튼 2개 동시에 눌리는 상황 처리(A. SW2&SW3 : LCD clear / B. SW5&SW6 : change writing mode / C. SW8&SW9 : make a space)
	if(shmaddr[2]==SW2 && shmaddr[3]==SW3){
		// LCD CLEAR
		for(i=1;i<=MAX_BUFF;i++)
			shmaddr[i]=32;// ' ' 공백 아스키 코드==32
		return 0;
	}
	else if(shmaddr[2]==SW5 && shmaddr[3]==SW6){
		// ENGLISH <-> NUMBER
		eng_num_flag=!eng_num_flag;	// flag 뒤집기
		// eng, num 모드 변경시 누른 스위치와 스위치 누른 횟수를 초기화해줌
		pushed_switch = '0';
		switch_counter = 0;
		input = 0;// update시 input==0은 별도 처리
		update_shm_mode3(shmaddr, string, input); // flag만 바꾸고 업데이트후 리턴. 다음 차례부터 바뀐모드로 입력받음
		return 0;
	}
	else if(shmaddr[2]==SW8 && shmaddr[3]==SW9){
		// MAKE A SPACE
		pushed_switch = '0';
		switch_counter = 0;
		input = 32; // 공백==32
		update_shm_mode3(shmaddr, string, input);
		return 0;
	}

//2. 글자 처리
	else{	// 하나의 스위치만 누른 경우	
		if(eng_num_flag==false){	// 영어 입력
			printf("-------영어 입력중----------\n");
// 누른 키를 다시 누르는 경우
			if(shmaddr[2]==pushed_switch){	// 눌렀던거 또 누르는 경우
				switch_counter++;
				if(switch_counter>3) switch_counter=1;	// 4번째 누르는 경우 처음 누른 것으로 판단
				switch(pushed_switch){
					case SW1 : 
						switch(switch_counter){
							case 1 :
								input = '.';
								break;
							case 2 :
								input = 'Q';
								break;
							case 3 :
								input = 'Z';
								break;
						}
						break;
					case SW2 : 
						switch(switch_counter){
							case 1 :
								input = 'A';
								break;
							case 2 :
								input = 'B';
								break;
							case 3 :
								input = 'C';
								break;
						}
						break;
					case SW3 : 
						switch(switch_counter){
							case 1 :
								input = 'D';
								break;
							case 2 :
								input = 'E';
								break;
							case 3 :
								input = 'F';
								break;
						}
						break;
					case SW4 : 
						switch(switch_counter){
							case 1 :
								input = 'G';
								break;
							case 2 :
								input = 'H';
								break;
							case 3 :
								input = 'I';
								break;
						}
						break;
					case SW5 : 
						switch(switch_counter){
							case 1 :
								input = 'J';
								break;
							case 2 :
								input = 'K';
								break;
							case 3 :
								input = 'L';
								break;
						}
						break;
					case SW6 : 
						switch(switch_counter){
							case 1 :
								input = 'M';
								break;
							case 2 :
								input = 'N';
								break;
							case 3 :
								input = 'O';
								break;
						}
						break;
					case SW7 : 
						switch(switch_counter){
							case 1 :
								input = 'P';
								break;
							case 2 :
								input = 'R';
								break;
							case 3 :
								input = 'S';
								break;
						}
						break;
					case SW8 : 
						switch(switch_counter){
							case 1 :
								input = 'T';
								break;
							case 2 :
								input = 'U';
								break;
							case 3 :
								input = 'V';
								break;
						}
						break;
					case SW9 : 
						switch(switch_counter){
							case 1 :
								input = 'W';
								break;
							case 2 :
								input = 'X';
								break;
							case 3 :
								input = 'Y';
								break;
						}
						break;
					default : 	// NO_SWITCH -> 아무것도 안바꾸고 업데이트.
						input = 0;
						break;
				}//END OF SWITCH
				previous_input = input;
			}// if문 끝 : 눌렀던거 또 누르는 경우
			else{	// 새로운 스위치가 눌림
				new_char = true;
				if(pushed_switch=='0'){	// 눌린 스위치 초기화하는 경우 - 공백넣기, 숫자영어모드 바꾸기 // 모드진입후최초입력
					// 최초입력
					if(first_input==false){
						first_input=true;
						switch(shmaddr[2]){
							case SW1 :
								input = '.';
								break;
							case SW2 :
								input = 'A';
								break;
							case SW3 :
								input = 'D';
								break;
							case SW4 :
								input = 'G';
								break;
							case SW5 :
								input = 'J';
								break;
							case SW6 :
								input = 'M';
								break;
							case SW7 :
								input = 'P';
								break;
							case SW8 :
								input = 'T';
								break;
							case SW9 :
								input = 'W';
								break;
						}
						switch_counter = 1;
					}
					// 공백, 숫자영어 변경하고 여기로 들어오는 경우
					else{
						switch_counter = 0;
					}
				}
				else{	// 새로운 스위치 처리
					pushed_switch = shmaddr[2];
					switch_counter = 1;
				}
			}// else문 끝 : 새로운 스위치 누르는 경우

			if(new_char == false){	// 같은 스위치를 계속 누르는 경우
				;	// 따로 할일은 없음. update_shm_mode3에 기존 string과 바뀐input만 넘김
			}
			else{					// 이전과 다른 스위치가 눌린 경우
				strcat(string,&previous_input);	// 이전 글자를 string에 합치고 새로운 input을 넘김
				new_char = false;	// 다시 플래그 초기화
			}
		}// if문 끝 : 영어 입력

		else{						// 숫자 입력
			printf("-------숫자 입력중----------\n");
			switch(pushed_switch){
				case SW1 : 
					input = '1';
					break;
				case SW2 : 
					input = '2';
					break;
				case SW3 : 
					input = '3';
					break;
				case SW4 : 
					input = '4';
					break;
				case SW5 : 
					input = '5';
					break;
				case SW6 : 
					input = '6';
					break;
				case SW7 : 
					input = '7';
					break;
				case SW8 : 
					input = '8';
					break;
				case SW9 : 
					input = '9';
					break;
				default :	// NO_SWITCH. 저장된 값들로 shm을 update
					input = 0;
					break;
			}// end of switch
		}// else문 끝 : 숫자 입력

	}// END OF else문 : 하나의 스위치만 누른 경우

	//update전
	printf("update전 shmaddr\n");
	for(i=0;i<=MAX_BUFF;i++)
		printf("%d ",shmaddr[i]);
	printf("\n");
	printf("update전 string : %s\n", string);
	printf("update전 input  : %d\n\n", input);

	//3. 마지막에 update_shm_mode3()수행
	update_shm_mode3(shmaddr, string, input);
	// update 후
	printf("update후 shmaddr\n");
	for(i=0;i<=MAX_BUFF;i++)
		printf("%d ",shmaddr[i]);
	printf("\n");
	printf("update후 string : %s\n", string);
	printf("update후 input  : %d\n\n", input);

	enter_mode3++;// mode3다녀감
	return 0;
}
*/
