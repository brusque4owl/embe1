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
#include "readkey.h"

#define BUFF_SIZE 64

#define KEY_RELEASE 0
#define KEY_PRESS 1

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
	//printf("----------code = %d\n",code);
while(!code){
	if( (rd = read (fd, ev, size * BUFF_SIZE)) >= size){ //for read nonblocking
		value = ev[0].value;

		if (value != ' ' && ev[1].value == 1 && ev[1].type == 1){ // Only read the key press event
			printf ("code%d\n", (ev[1].code));
		}
		/*
		if( value == KEY_RELEASE ) {
			printf ("key release\n");
		} else 				if( value == KEY_PRESS ) {
			printf ("key press\n");
		}
		*/
		//printf ("Type[%d] Value[%d] Code[%d]\n", ev[0].type, ev[0].value, (ev[0].code));
		code = ev[0].code;
		//printf("code = %d\n", code);
	}
}// while(!code)
	close(fd);
	return code;
}
