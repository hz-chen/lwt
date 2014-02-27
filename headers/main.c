#include <stdio.h>
#include <stdlib.h>
#include "lwt.h"

#define CHAN_SZ	10


//a seperate kthd fn generate input 
void* fn_generate(void* data, lwt_chan_t c)
{
	//rcv a chennel from fn_process
	lwt_chan_t snd_cc = lwt_rcv_chan(c);

	int num = 10;
	while(num > 0)
	{
		printf("number %d generated.\n", num);
		lwt_snd(snd_cc, (void*)num);
		num--;
	}

	return NULL;
}

//fn on many lwt thread on a kthd to process user input
void* fn_process(void* data, lwt_chan_t c)
{
	//this channel will be send to fn_generate
	lwt_chan_t rcv_cc = lwt_chan(CHAN_SZ);
	lwt_snd_chan((lwt_chan_t)data, rcv_cc);

	//rcv a channel from fn_print
	lwt_chan_t snd_cc = lwt_rcv_chan(c);

	int tmp;
	while((tmp = (int)lwt_rcv(rcv_cc)))
	{
		if(tmp == -1)
			break;
		int i=0;
		for(;i<100;i++)
		{
			tmp++;
			if (i%5==0)
				lwt_yield(_LWT_NULL);
		}

		printf("number %d processed\n", tmp);
		lwt_snd(snd_cc, (void*)tmp);
	}
	return NULL;
}

//fn on a kthd to print processed data
void* fn_print(void* data, lwt_chan_t c)
{

	//this channel will be send to fn_process
	lwt_chan_t rcv_cc = lwt_chan(CHAN_SZ);
	lwt_snd_chan((lwt_chan_t)data, rcv_cc);

	int tmp;
	while((tmp = (int)lwt_rcv(rcv_cc)) != -1)
	{
		printf("after calculation, result is: %d\n", tmp);
	}


	return NULL;
}

int main(int argc, char** argv)
{
	//main channel used for inter thread communication
	lwt_chan_t main_c = lwt_chan(CHAN_SZ);
	
	lwt_chan_t generate_c = lwt_chan(CHAN_SZ);
	lwt_kthd_create(&fn_generate, (void*)main_c, generate_c);

	lwt_chan_t process_c = lwt_chan(CHAN_SZ);
	lwt_kthd_create(&fn_process, (void*)main_c, process_c);

	//fn_process will send a channel to fn_generate:
	lwt_snd_chan(generate_c, lwt_rcv_chan(main_c));

	lwt_chan_t print_c = lwt_chan(CHAN_SZ);
	lwt_kthd_create(&fn_print, (void*)main_c, print_c);

	//fn_print will send a channel to fn_process
	lwt_snd_chan(process_c, lwt_rcv_chan(main_c));


	sleep(1);

	return EXIT_SUCCESS;
}
