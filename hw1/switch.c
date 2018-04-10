#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <string.h>

#include "switch.h"

#define MAX_BUTTON 9
#define SWITCH_PUSHED 1

int read_switch(char *sw_buff)
{
	int i, push_count=0;	// push_count는 눌린 버튼 개수
	int dev;
	int buff_size;

	char push_sw_buff[MAX_BUTTON]={0,};

	dev = open("/dev/fpga_push_switch", O_RDWR);
	
	if(dev<0){
		printf("Device Open Error\n");
		close(dev);
		return -1;
	}

	buff_size=sizeof(push_sw_buff);
	read(dev,push_sw_buff,buff_size);
	for(i=0;i<MAX_BUTTON;i++){
		if(push_sw_buff[i]==SWITCH_PUSHED){
			push_count++;
		}
		printf("[%d] ", sw_buff[i]);
	}
	printf("%d\tbuff size = %d\tread return = %d\n",push_count, buff_size,read_return);

	strcpy(sw_buff, push_sw_buff);
	close(dev);
	return push_count;
}// end of read_switch()
