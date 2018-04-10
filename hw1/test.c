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
#include <string.h>
#include "readkey.h"

#define BUFF_SIZE 64

#define KEY_RELEASE 0
#define KEY_PRESS 1

#define MAX_BUTTON 9

int main(void){
	int fd_key, fd_switch, i, code;
	int switch_count=0;
	char *buff = (char *)malloc(sizeof(char)*MAX_BUTTON);
	int buff_size, count=0;

	struct input_event ev[BUFF_SIZE];
	int rd, value, size = sizeof (struct input_event);
//open key
	char* key_dev = "/dev/input/event0";
	if((fd_key = open (key_dev, O_RDONLY|O_NONBLOCK)) == -1) {  //for read nonblocking
		printf ("%s is not a vaild device.n", key_dev );
	}

//open switch
	unsigned char push_sw_buff[MAX_BUTTON];
	fd_switch = open("/dev/fpga_push_switch", O_RDWR);
	if(fd_switch<0){
		printf("Device Open Error\n");
		close(fd_key);
		return -1;
	}
while(1){
// read key
	if( (rd = read (fd_key, ev, size * BUFF_SIZE)) >= size){ //for read nonblocking
		value = ev[0].value;

		if (value != ' ' && ev[1].value == 1 && ev[1].type == 1){ // Only read the key press event
			printf ("code%d\n", (ev[1].code));
		}
		code = ev[0].code;
		printf("key = %d\n", code);

	}

// read switch
	buff_size = sizeof(push_sw_buff);
	read(fd_switch, &push_sw_buff, buff_size);
	for(i=0;i<MAX_BUTTON;i++){
		if(push_sw_buff[i]==1)
			switch_count++;
		//printf("[%d] ", push_sw_buff[i]);
	}
	//printf("\n");
	if(switch_count>0){
		for(i=0;i<MAX_BUTTON;i++){
			printf("[%d] ", push_sw_buff[i]);
		}
		printf("\n");
	
	}
}// end of while
	close(fd_key);
	close(fd_switch);

	return 0;
}
