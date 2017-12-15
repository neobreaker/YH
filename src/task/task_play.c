#include "ucos_ii.h"
#include "stm32f10x.h"
#include "task_startup.h"
#include "vs10xx_port.h"
#include "task_udpserver.h"
#include "task_play.h"
#include "queue.h"

extern Queue *s_rcv_queue;
extern vs10xx_cfg_t g_vs10xx_play_cfg;

void play()
{
	u8 *pbuff;
	int len = 0;
	rev_buffer_t *prcv_buff = NULL;
	int i = 0;

	pbuff = pvPortMalloc(RCV_BUFFER_SIZE);
	
	VS_Restart_Play(&g_vs10xx_play_cfg);  					//重启播放 
	VS_Set_All(&g_vs10xx_play_cfg);        					//设置音量等信息 			 
	VS_Reset_DecodeTime(&g_vs10xx_play_cfg);
	
	while(1)
	{
		prcv_buff = rcv_queue_dequeue();
		len = prcv_buff->len;
		prcv_buff->len = 0;
		rcv_queue_enqueue(prcv_buff);
		if(prcv_buff == NULL || len > 0)
		{
			memcpy(pbuff, prcv_buff->data, len);
			g_vs10xx_play_cfg.VS_SPI_SpeedHigh();	//高速		
			for(i = 0; i < len-32; i+=32)
			{
				VS_Send_MusicData(&g_vs10xx_play_cfg, pbuff+i);
				
			}
			if(i < len)
			{
				VS_Send_MusicData2(&g_vs10xx_play_cfg, pbuff+i, len -i);
			}
		}
	}
	
}

void task_play(void *p_arg)
{
	
	while(1)
	{
		play();
		//OSTimeDly(1000);
	}
	
}

