#include "ucos_ii.h"
#include "stm32f10x.h"
#include "task_startup.h"
#include "lwip/sockets.h"  
#include "lwip/err.h"
#include "lib_mem.h"
#include "task_udpserver.h"
#include "queue.h"

extern OS_EVENT* sem_vs1053async;

struct sockaddr_in g_remote_sin;

u8 is_line_established = 0;		//通讯连接是否建立

static rev_buffer_t s_rcv_buffer[RCV_BUFFER_NUM];
static Queue *s_rcv_queue = NULL;

static void rcv_queue_enqueue(void *data)
{
	if(queue_size(s_rcv_queue) == RCV_BUFFER_NUM)
		queue_poll(s_rcv_queue, NULL);
	
	queue_enqueue(s_rcv_queue, data);
}

static void* rcv_queue_dequeue(void *data)
{
	void *ret = NULL;

	queue_poll(s_rcv_queue, &ret);

	return ret;
}

static void rev_buffer_init()
{
	int i = 0; 
	
	queue_new(&s_rcv_queue);
	
	for (i = 0; i < RCV_BUFFER_NUM; i++)
	{
		s_rcv_buffer[i].data = pvPortMalloc(RCV_BUFFER_SIZE);
		if(s_rcv_buffer[i].data == NULL)
			return;
		rcv_queue_enqueue(&s_rcv_buffer[i]);
		s_rcv_buffer[i].len = 0;
	}
}

void task_udpserver(void *p_arg)
{
	int err;	
	struct sockaddr_in sin;
	socklen_t sin_len; 
	int sock_fd;                /* server socked */
	int ret = 0;
	int len = 0;
	rev_buffer_t *pbuff = NULL;
	
	rev_buffer_init();
	
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
		pbuff = rcv_queue_dequeue(s_rcv_queue);
		pbuff->len = recvfrom(sock_fd, pbuff->data, RCV_BUFFER_SIZE, 0, (struct sockaddr *)&g_remote_sin, &sin_len);
		ret = parse_AT(pbuff->data, RCV_BUFFER_SIZE);
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


