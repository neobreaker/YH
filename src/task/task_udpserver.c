#include "ucos_ii.h"
#include "stm32f10x.h"
#include "task_startup.h"
#include "lwip/sockets.h"  
#include "lwip/err.h"
#include "lib_mem.h"

static u8 data_buffer[256];

static u32 time_stamp = 0, time_elapse = 0;
void task_udpserver(void *p_arg)
{
	int err;	
	struct sockaddr_in sin;
	socklen_t sin_len; 
	int sock_fd;                /* server socked */
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
    sin.sin_port=htons(50000);
	sin_len = sizeof(sin);
	
	err = bind(sock_fd, (struct sockaddr *)&sin, sizeof(sin));  
    if (err < 0) 
	{  
        return ;  
    }
	
	while(1)
	{
		recvfrom(sock_fd, data_buffer, sizeof(data_buffer), 0, (struct sockaddr *)&sin, &sin_len);
		time_stamp = OSTimeGet();
		for(i = 0; i < 1024; i++)
		{
			sendto(sock_fd, snddata, 1024, 0, (struct sockaddr *)&sin, sizeof(sin)); 
		}
		time_elapse = OSTimeGet() - time_stamp;
		time_elapse = 0;
	}
	
}


