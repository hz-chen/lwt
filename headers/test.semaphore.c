#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include "lwt.h"

#define NITER 1000000

int count = 0;

sem_t sem;
void * ThreadAdd(void * a, lwt_chan_t c)
{

	int i, tmp;
	for(i = 0; i < NITER; i++)
	{
		sem_wait(&sem);
		tmp = count;      /* copy the global count locally */
		tmp = tmp+1;      /* increment the local copy */
		count = tmp;      /* store the local value into the global count */ 
		sem_post(&sem);
	}
	return NULL;
}

int main(int argc, char * argv[])
{
	sem_init(&sem, 0, 1);

	lwt_kthd_create(ThreadAdd, NULL, NULL);
	lwt_kthd_create(ThreadAdd, NULL, NULL);

	sleep(1);

	sem_destroy(&sem);
	if (count < 2 * NITER) 
		printf("\n BOOM! count is [%d], should be %d\n", count, 2*NITER);
	else
		printf("\n OK! count is [%d]\n", count);

	return EXIT_SUCCESS;
}



