#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <linux/input.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <signal.h>
#include "readkey.h"

#define BUFF_SIZE 64

#define KEY_RELEASE 0
#define KEY_PRESS 1

#define MAX_BUTTON 9

int readkey (void)
{
	struct input_event ev[BUFF_SIZE];
	int fd, rd, value, size = sizeof (struct input_event);
	int code=0;

	char* device = "/dev/input/event0";
	if((fd = open (device, O_RDONLY|O_NONBLOCK)) == -1) {  //for read nonblocking
		printf ("%s is not a vaild device.n", device);
	}
	// ioctl (fd, EVIOCGNAME (sizeof (name)), name);
	// printf ("Reading From : %s (%s)n", device, name);
	if( (rd = read (fd, ev, size * BUFF_SIZE)) >= size){ //for read nonblocking
		value = ev[0].value;

		if (value != ' ' && ev[1].value == 1 && ev[1].type == 1){ // Only read the key press event
			printf ("code%d\n", (ev[1].code));
		}
		//printf ("Type[%d] Value[%d] Code[%d]\n", ev[0].type, ev[0].value, (ev[0].code));
		code = ev[0].code;
	}
	close(fd);
	if(rd==-1) return -1;
	return code;
}

int readswitch(char *buff){
	int i;
	int dev;
	int buff_size, count=0;
	unsigned char push_sw_buff[MAX_BUTTON];
	dev = open("/dev/fpga_push_switch", O_RDWR);
	if(dev<0){
		printf("Device Open Error\n");
		close(dev);
		return -1;
	}

	buff_size = sizeof(push_sw_buff);
	read(dev, &push_sw_buff, buff_size);
	for(i=0;i<MAX_BUTTON;i++){
		if(push_sw_buff[i]==1)
			count++;
	}
	/*
	for(i=0;i<MAX_BUTTON;i++){
		printf("[%d] ",push_sw_buff[i]);
	}
	printf("\n");
	*/
	strcpy(buff, push_sw_buff);
	close(dev);
	return count;
}

int main(void){
	int code;
	int switch_count;
	char *buff = (char *)malloc(sizeof(char)*MAX_BUTTON);
	while(1){
		code = readkey();
		switch_count = readswitch(buff);
		if(code!=-1){printf("pushed code = %d\n",code); /*break;*/}		// 눌린 키가 있으면 나가기
		if(switch_count>0){printf("switch contents = %s\n",buff); /*break;*/}// 스위치 눌리면 나가기
	}
	return 0;
}
