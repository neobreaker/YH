#include "ucos_ii.h"
#include "stm32f10x.h"
#include "task_startup.h"
#include "sram.h"
#include "stdio.h"
#include "lib_mem.h"
#include "array.h"
#include "lwip_comm.h"
#include "delay.h"

//////////////////////////////////////////////////////////////////
//加入以下代码,支持printf函数,而不需要选择use MicroLIB	  
#if 1
#pragma import(__use_no_semihosting)             
//标准库需要的支持函数                 
struct __FILE 
{ 
	int handle; 

}; 

FILE __stdout;       
//定义_sys_exit()以避免使用半主机模式    
_sys_exit(int x) 
{ 
	x = x; 
} 
//重定义fputc函数 
int fputc(int ch, FILE *f)
{      
	//while((USART1->SR&0X40)==0);//循环发送,直到发送完毕   
    //USART1->DR = (u8) ch;      
	return ch;
}
#endif 


static OS_STK startup_task_stk[STARTUP_TASK_STK_SIZE];

void bsp_init()
{
	NVIC_Configuration();
	delay_init();
	FSMC_SRAM_Init();
}

void array_test()
{
    size_t i = 0;
    int *pv = NULL;
    int val[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    Array *ar = NULL;

    array_new(&ar);

    for(i = 0 ; i < sizeof(val)/ sizeof(int); i++)
    {
        array_add(ar, &val[i]);
    }

    for(i = 0 ; i < sizeof(val)/ sizeof(int); i++)
    {
        array_get_at(ar, i, (void**)&pv);
    }

    array_destroy(ar);

}

int main(void)
{
	bsp_init();
	
	OSInit();

	lwip_comm_init();
	
	OSTaskCreate(startup_task, (void *)0,
	             &startup_task_stk[STARTUP_TASK_STK_SIZE - 1],
	             STARTUP_TASK_PRIO);
	OSStart();
	return 0;
}
