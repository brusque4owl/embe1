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

	dev = open("/dev/fpga_push_switch", O_RDONLY|O_NONBLOCK);
	
	if(dev<0){
		printf("Device Open Error\n");
		close(dev);
		return -1;
	}

	buff_size=sizeof(push_sw_buff);
	//buff_size=sizeof(sw_buff);

	//while(1){
		read(dev,push_sw_buff,buff_size);
		for(i=0;i<MAX_BUTTON;i++){
			if(push_sw_buff[i]==SWITCH_PUSHED){
				sw_buff[i]=push_sw_buff[i];
				push_count++;
			}
			//printf("[%d] ", sw_buff[i]);
		}
		//printf("push_count = %d\n",push_count);
	//}//while(!code)
	close(dev);
	return push_count;
}// end of read_switch()
