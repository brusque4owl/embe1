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

int mode4(char *shmaddr){

}
