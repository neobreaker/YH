#include "ucos_ii.h"
#include "stm32f10x.h"
#include "task_startup.h"
#include "vs10xx_play_port.h"
#include "task_udpserver.h"
#include "task_play.h"
#include "queue.h"
#include "wav.h"

extern Queue *s_rcv_queue;
extern vs10xx_cfg_t g_vs10xx_play_cfg;
extern OS_EVENT* sem_vs1053_play_async;
extern u8 is_line_established;

u8 vs10xx_play_dma_buff[SEND_NUM_PER_FRAME];

void vs10xx_send_data(u8* pbuff, int len)
{
	int i = 0;
	for(i = 0; i < len-SEND_NUM_PER_FRAME; i+=SEND_NUM_PER_FRAME)
    {
        VS_Send_MusicData(&g_vs10xx_play_cfg, pbuff+i);
		
    }
    if(i < len)
    {
        VS_Send_MusicData2(&g_vs10xx_play_cfg, pbuff+i, len -i);
    }
}

void vs10xx_play_dma_send_data(u8* pbuff, int len)
{
	int i = 0;
	for(i = 0; i < len-SEND_NUM_PER_FRAME; i+=SEND_NUM_PER_FRAME)
    {
        memcpy(vs10xx_play_dma_buff, pbuff+i, SEND_NUM_PER_FRAME);
		if(g_vs10xx_play_cfg.VS_DQ() != 0)  //送数据给VS10XX
    	{
	        g_vs10xx_play_cfg.VS_XDCS(0);
			SPI_I2S_DMACmd(SPI3, SPI_I2S_DMAReq_Tx, ENABLE);
			vs10xx_play_dma_enable(SEND_NUM_PER_FRAME);
			
		}
		OSTimeDly(2);
		g_vs10xx_play_cfg.VS_XDCS(1);
    }
    if(i < len)
    {
        memcpy(vs10xx_play_dma_buff, pbuff+i, len -i);
		if(g_vs10xx_play_cfg.VS_DQ() != 0)  //送数据给VS10XX
    	{
	        g_vs10xx_play_cfg.VS_XDCS(0);
			SPI_I2S_DMACmd(SPI3, SPI_I2S_DMAReq_Tx, ENABLE);
			vs10xx_play_dma_enable(len -i);
		}
		OSTimeDly(2);
    }
}

void play()
{
    u8 *pbuff;
    int len = 0;
    rev_buffer_t *prcv_buff = NULL;
	u8 _err = 0;
	WaveHeaderStruct wh;
	
    pbuff = pvPortMalloc(RCV_BUFFER_SIZE);

	wav_header_init(&wh, 512+36, 512);

    VS_Restart_Play(&g_vs10xx_play_cfg);                    //重启播放
    VS_Set_All(&g_vs10xx_play_cfg);                         //设置音量等信息
    VS_Reset_DecodeTime(&g_vs10xx_play_cfg);
    g_vs10xx_play_cfg.VS_SPI_SpeedHigh();   //高速
    vs10xx_send_data((u8*)&wh, sizeof(WaveHeaderStruct));
    while(1)
    {
        OSSemPend(sem_vs1053_play_async, 0, &_err);

        if(_err == OS_ERR_NONE)
        {
			
            while(1)
            {
                prcv_buff = rcv_queue_dequeue();
                len = prcv_buff->len;
                prcv_buff->len = 0;
                rcv_queue_enqueue(prcv_buff);
                if(prcv_buff == NULL || len > 0)
                {
					if(prcv_buff->data[0] == 'R' 
						&& prcv_buff->data[1] == 'I' 
						&& prcv_buff->data[2] == 'F' 
						&& prcv_buff->data[2] == 'F' )		//skip wav header
					{	
						len = 0;
						continue;
					}
					
                    memcpy(pbuff, prcv_buff->data, len);
                    g_vs10xx_play_cfg.VS_SPI_SpeedHigh();   //高速
                    vs10xx_play_dma_send_data(pbuff, len);
                    
                }

				if(!is_line_established)			//通讯中断
					break;
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

