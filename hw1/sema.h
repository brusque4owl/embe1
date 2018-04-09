#include <sys/sem.h>

union semun{
	int val;
	struct semid_ds *buf;
	unsigned short *array;
};

int initsem(key_t semkey, int val);
int semlock(int semid);
int semunlock(int semid);
