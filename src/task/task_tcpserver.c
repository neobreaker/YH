#include "ucos_ii.h"
#include "stm32f10x.h"
#include "task_startup.h"
#include "lwip/sockets.h"  
#include "lwip/err.h"


void tcpserver_init()
{
	
}

static u8 data_buffer[256];

void task_tcpserver(void *p_arg)
{
	int err;
	struct sockaddr_in server_addr;  
    struct sockaddr_in conn_addr;
	int sock_fd;                /* server socked */
	int sock_conn;          	/* request socked */
	socklen_t addr_len;
	int length;

	sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd == -1) 
	{  
        return ;
    }

	server_addr.sin_family = AF_INET;  
    server_addr.sin_addr.s_addr =htonl(INADDR_ANY);  
    server_addr.sin_port = htons(50000);

	err = bind(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));  
    if (err < 0) 
	{  
        return ;  
    }

	err = listen(sock_fd, 1);  
    if (err < 0) {  
        return ;  
    }

	sock_conn = accept(sock_fd, (struct sockaddr *)&conn_addr, &addr_len);
	
	while(1)
	{
		length = recv(sock_conn, (unsigned int *)data_buffer, 256, 0);
		send(sock_conn, "good", 5, 0);
	}
}

