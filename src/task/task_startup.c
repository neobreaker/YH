#include "ucos_ii.h"
#include "stm32f10x.h"
#include "task_startup.h"

static OS_STK task_tcpserver_stk[TASK_TCPSERVER_STK_SIZE];
static OS_STK task_udpserver_stk[TASK_TCPSERVER_STK_SIZE];

extern void task_tcpserver(void *p_arg);
extern void task_udpserver(void *p_arg);

static void systick_init(void)
{
	RCC_ClocksTypeDef rcc_clocks;
	RCC_GetClocksFreq(&rcc_clocks);
	SysTick_Config(rcc_clocks.HCLK_Frequency / OS_TICKS_PER_SEC);
}

void startup_task(void *p_arg)
{
	INT8U err;
	systick_init();     /* Initialize the SysTick. */

#if (OS_TASK_STAT_EN > 0)
    OSStatInit();      /* Determine CPU capacity. */
#endif
	/* TODO: create application tasks here */

	lwip_comm_init();
	
	/*
	err = OSTaskCreate(task_tcpserver, (void *)0,
	                   &task_tcpserver_stk[TASK_TCPSERVER_STK_SIZE-1], TASK_TCPSERVER_PRIO);      
	*/

	err = OSTaskCreate(task_udpserver, (void *)0,
	                   &task_udpserver_stk[TASK_UDPSERVER_STK_SIZE-1], TASK_UDPSERVER_PRIO);      

	if (OS_ERR_NONE != err)
		while(1)
			;
	OSTaskDel(OS_PRIO_SELF);
}

