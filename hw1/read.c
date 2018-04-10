#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <linux/input.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/time.h>
#include <termios.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <string.h>
#include "switch.h"
#include "readkey.h"

#define BUFF_SIZE 64

#define KEY_RELEASE 0
#define KEY_PRESS 1
#define MAX_BUTTON 9
#define SWITCH_PUSHED 1
int read_key_sw (char *sw_buff)
{
	//for key
	struct input_event ev[BUFF_SIZE];
	int fd, rd, value, size = sizeof (struct input_event);
	int code=0;

	//for switch
	int i, push_count=0;	// push_count는 눌린 버튼 개수
	int dev;
	int buff_size;
	char push_sw_buff[MAX_BUTTON]={0,};

//key device
	char* device = "/dev/input/event0";
	if((fd = open (device, O_RDONLY|O_NONBLOCK)) == -1) {  //for read nonblocking
		printf ("%s is not a vaild device.n", device);
	}
//switch device
	dev = open("/dev/fpga_push_switch", O_RDONLY);
	if(dev<0){
		printf("Device Open Error\n");
		close(dev);
		return -1;
	}

while(1){	// read key or switch
//key dev code
	if( (rd = read (fd, ev, size * BUFF_SIZE)) </*>=*/ size){ //for read nonblocking
		value = ev[0].value;

		if (value != ' ' && ev[1].value == 1 && ev[1].type == 1){ // Only read the key press event
			printf ("code%d\n", (ev[1].code));
		}
		//printf ("Type[%d] Value[%d] Code[%d]\n", ev[0].type, ev[0].value, (ev[0].code));
		code = ev[0].code;
		//if(rd!=-1)
		if(code!=0){close(fd); close(dev); return code;}
	}
//switch dev code
	else{
		buff_size=sizeof(push_sw_buff);
		read(dev,push_sw_buff,buff_size);
		for(i=0;i<MAX_BUTTON;i++){
			if(push_sw_buff[i]==SWITCH_PUSHED){
				sw_buff[i]=push_sw_buff[i];
				push_count++;
			}
			//printf("[%d] ", sw_buff[i]);
		}
		if(push_count!=0){close(fd); close(dev); return 0;}
	}
}// END OF WHILE(1)
}
