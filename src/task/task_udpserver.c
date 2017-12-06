#include "ucos_ii.h"
#include "stm32f10x.h"
#include "task_startup.h"
#include "lwip/sockets.h"  
#include "lwip/err.h"
#include "lib_mem.h"
#include "task_udpserver.h"

extern OS_EVENT* sem_vs1053async;

static u8 data_buffer[RCV_BUFFER_SIZE];

struct sockaddr_in g_remote_sin;

u8 is_line_established = 0;		//通讯连接是否建立

void task_udpserver(void *p_arg)
{
	int err;	
	struct sockaddr_in sin;
	socklen_t sin_len; 
	int sock_fd;                /* server socked */
	int ret = 0;
	
	sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_fd == -1) 
	{  
        return ;
    }

	sin.sin_family=AF_INET;  
    sin.sin_addr.s_addr=htonl(INADDR_ANY);  
    sin.sin_port=htons(SRC_RCV_PORT);
	sin_len = sizeof(sin);
	
	err = bind(sock_fd, (struct sockaddr *)&sin, sizeof(sin));  
    if (err < 0) 
	{  
        return ;  
    }
	
	while(1)
	{
		recvfrom(sock_fd, data_buffer, sizeof(data_buffer), 0, (struct sockaddr *)&g_remote_sin, &sin_len);
		ret = parse_AT(data_buffer, RCV_BUFFER_SIZE);
		if(ret && is_line_established)
		{
			OSSemPost(sem_vs1053async);
		}

		/*
		for(i = 0; i < 1024; i++)
		{
			sendto(sock_fd, snddata, 1024, 0, (struct sockaddr *)&g_remote_sin, sizeof(sin)); 
		}
		*/
	}
	
}

//return 0 : NOT AT  1: AT
int parse_AT(u8* buffer, int len)
{
	u8 cmd = 0;
	if(buffer[0] == 'A' && buffer[1] == 'T')
	{
		cmd = buffer[2];

		switch(cmd)
		{
		case AT_LINE_EATABLISH:
			is_line_established = 1;
			break;
		case AT_LINE_SHUTDOWN:
			is_line_established = 0;
			break;
		}
		return 1;
	}
	return 0;
}


