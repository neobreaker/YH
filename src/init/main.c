#include "ucos_ii.h"
#include "stm32f10x.h"
#include "task_startup.h"
#include "sram.h"
#include "stdio.h"
#include "lib_mem.h"
#include "array.h"
#include "lwip_comm.h"
#include "delay.h"
#include "ccdebug.h"

static OS_STK startup_task_stk[STARTUP_TASK_STK_SIZE];

void bsp_init()
{
	NVIC_Configuration();
	delay_init();
	FSMC_SRAM_Init();
	ccdebug_port_init(9600);
}

int main(void)
{
	bsp_init();
	
	delay_ms(1000);		/* wait power stable */ 
	
	OSInit();

	CC_DEBUGF(CC_DBG_ON | CC_DBG_LEVEL_WARNING, "system booting\n");
	
	OSTaskCreate(startup_task, (void *)0,
	             &startup_task_stk[STARTUP_TASK_STK_SIZE - 1],
	             STARTUP_TASK_PRIO);
	OSStart();
	
	CC_DEBUGF(CC_DBG_ON | CC_DBG_LEVEL_WARNING, "system shutdonwn\n");		/* should never run here! */
	
	return 0;
}
