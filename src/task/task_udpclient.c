#include "ucos_ii.h"
#include "stm32f10x.h"
#include "task_startup.h"
#include "lwip/sockets.h"  
#include "lwip/err.h"
#include "lib_mem.h"
#include "task_udpclient.h"

extern OS_EVENT* sem_vs1053async;

extern struct sockaddr_in g_remote_sin;

static u32 time_stamp = 0, time_elapse = 0;
void task_udpclient(void *p_arg)
{
	int err;	
	u8 _err;
	struct sockaddr_in sin;
	socklen_t sin_len; 
	int sock_fd;                
	u8* snddata;
	int i = 0;
	
	snddata = pvPortMalloc(1024);
	memset(snddata, 0x5a, 1024);
	
	sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_fd == -1) 
	{  
        return ;
    }

	sin.sin_family=AF_INET;  
    sin.sin_addr.s_addr=htonl(INADDR_ANY);  
    sin.sin_port=htons(SRC_SND_PORT);
	sin_len = sizeof(sin);
	
	err = bind(sock_fd, (struct sockaddr *)&sin, sizeof(sin));  
    if (err < 0) 
	{  
        return ;  
    }
	
	while(1)
	{
		OSSemPend(sem_vs1053async, 0, &_err);
	    if(_err == OS_ERR_NONE)
	    {
			sin.sin_addr.s_addr = g_remote_sin.sin_addr.s_addr;

			time_stamp = OSTimeGet();
			for(i = 0; i < 1024; i++)
			{
				sendto(sock_fd, snddata, 1024, 0, (struct sockaddr *)&sin, sizeof(sin)); 
			}
			time_elapse = OSTimeGet() - time_stamp;
			time_elapse = 0;
	    }
	}
	
}


