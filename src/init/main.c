#include "ucos_ii.h"
#include "stm32f10x.h"
#include "task_startup.h"
#include "sram.h"
#include "stdio.h"
#include "lib_mem.h"
#include "array.h"

static OS_STK startup_task_stk[STARTUP_TASK_STK_SIZE];

void bsp_init()
{
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
        //printf("index %d is %d\r", i, *pv);
    }

    array_destroy(ar);

}

int main(void)
{
	bsp_init();
	array_test();
	
	OSInit();
	OSTaskCreate(startup_task, (void *)0,
	             &startup_task_stk[STARTUP_TASK_STK_SIZE - 1],
	             STARTUP_TASK_PRIO);
	OSStart();
	return 0;
}
