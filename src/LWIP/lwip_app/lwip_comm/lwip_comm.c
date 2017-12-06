#include "lwip_comm.h"
#include "netif/etharp.h"
#include "lwip/dhcp.h"
#include "lwip/mem.h"
#include "lwip/memp.h"
#include "lwip/init.h"
#include "ethernetif.h"
#include "lwip/timers.h"
#include "lwip/tcp_impl.h"
#include "lwip/ip_frag.h"
#include "lwip/tcpip.h"
#include "lib_mem.h"
#include <stdio.h>
#include "includes.h"
#include "ccdebug.h"

//////////////////////////////////////////////////////////////////////////////////
//������ֻ��ѧϰʹ�ã�δ��������ɣ��������������κ���;
//ALIENTEK ս�������� V3
//lwipͨ������ ����
//����ԭ��@ALIENTEK
//������̳:www.openedv.com
//��������:2015/3/15
//�汾��V1.0
//��Ȩ���У�����ؾ���
//Copyright(C) ������������ӿƼ����޹�˾ 2009-2019
//All rights reserved
//*******************************************************************************
//�޸���Ϣ
//��
//////////////////////////////////////////////////////////////////////////////////

//��OS�汾��lwipʵ��,�����Ҫ3������: lwip�ں�����(������),dhcp����(��ѡ),DM9000��������(������)
//�������������񶼴���.
//����,����Ҫ�õ�2���ź���,���ں���ķ���DM9000.

//lwip DHCP����
//�����������ȼ�
#define LWIP_DHCP_TASK_PRIO             7
//���������ջ��С
#define LWIP_DHCP_STK_SIZE              128
//�����ջ�������ڴ����ķ�ʽ��������
OS_STK * LWIP_DHCP_TASK_STK = NULL;
//������
void lwip_dhcp_task(void *pdata);

//lwip DM9000���ݽ��մ�������
//�����������ȼ�
#define LWIP_DM9000_INPUT_TASK_PRIO     6
//���������ջ��С
#define LWIP_DM9000_INPUT_TASK_SIZE     512
//�����ջ�������ڴ����ķ�ʽ��������
OS_STK * LWIP_ENC28J60_INPUT_TASK_STK = NULL;
//������
void lwip_netcard_input_task(void *pdata);


OS_STK * TCPIP_THREAD_TASK_STK = NULL;         //lwip�ں������ջ

OS_EVENT* sem_enc28j60input;                        //�������������ź���
OS_EVENT* sem_enc28j60lock;                 		//������д���������ź���

//////////////////////////////////////////////////////////////////////////////////////////
__lwip_dev lwipdev;                     //lwip���ƽṹ��
struct netif lwip_netif;                //����һ��ȫ�ֵ�����ӿ�
extern u32 memp_get_memorysize(void);   //��memp.c���涨��
extern u8_t *memp_memory;               //��memp.c���涨��.
extern u8_t *ram_heap;                  //��mem.c���涨��.

//DM9000���ݽ��մ�������
void lwip_netcard_input_task(void *pdata)
{
    //�����绺�����ж�ȡ���յ������ݰ������䷢�͸�LWIP����
    ethernetif_input(&lwip_netif);
}

//lwip�ں˲���,�ڴ�����
//����ֵ:0,�ɹ�;
//    ����,ʧ��
u8 lwip_comm_mem_malloc(void)
{
    u32 mempsize;
	u32 ramheapsize; 
	
	mempsize = memp_get_memorysize();			//��?��?memp_memory��y�����䨮D?
	memp_memory = pvPortMalloc(mempsize);	//?amemp_memory����???����?

	ramheapsize=LWIP_MEM_ALIGN_SIZE(MEM_SIZE)+2*LWIP_MEM_ALIGN_SIZE(4*3)+MEM_ALIGNMENT;//��?��?ram heap�䨮D?
	ram_heap = pvPortMalloc(ramheapsize); //Ϊram_heap�����ڴ�

    TCPIP_THREAD_TASK_STK         = pvPortMalloc(TCPIP_THREAD_STACKSIZE*4);           //���ں����������ջ
    LWIP_ENC28J60_INPUT_TASK_STK  = pvPortMalloc(LWIP_DM9000_INPUT_TASK_SIZE*4);      //��dm9000�������������ջ

#if LWIP_DHCP
    LWIP_DHCP_TASK_STK          = pvPortMalloc(LWIP_DHCP_STK_SIZE*4);               //��dhcp���������ջ

	if(!LWIP_DHCP_TASK_STK)
	{
		lwip_comm_mem_free();
		return 1;
	}
#endif

    if(!memp_memory||!ram_heap||!TCPIP_THREAD_TASK_STK||!LWIP_ENC28J60_INPUT_TASK_STK)//������ʧ�ܵ�
    {
        lwip_comm_mem_free();
        return 1;
    }
    
    return 0;

}
//lwip�ں˲���,�ڴ��ͷ�
void lwip_comm_mem_free(void)
{
    
    vPortFree(ram_heap);
    vPortFree(TCPIP_THREAD_TASK_STK);
    vPortFree(LWIP_DHCP_TASK_STK);
    vPortFree(LWIP_ENC28J60_INPUT_TASK_STK);
}
//lwip Ĭ��IP����
//lwipx:lwip���ƽṹ��ָ��
void lwip_comm_default_ip_set(__lwip_dev *lwipx)
{
    //Ĭ��Զ��IPΪ:192.168.1.106
    lwipx->remoteip[0]=192;
    lwipx->remoteip[1]=168;
    lwipx->remoteip[2]=3;
    lwipx->remoteip[3]=106;
    //MAC��ַ����(�����ֽڹ̶�Ϊ:2.0.0,�����ֽ���STM32ΨһID)
    lwipx->mac[0]=2;
    lwipx->mac[1]=0;
    lwipx->mac[2]=0;
    lwipx->mac[3]=1;
    lwipx->mac[4]=2;
    lwipx->mac[5]=3;
    //Ĭ�ϱ���IPΪ:192.168.1.30
    lwipx->ip[0]=192;
    lwipx->ip[1]=168;
    lwipx->ip[2]=2;
    lwipx->ip[3]=60;
    //Ĭ����������:255.255.255.0
    lwipx->netmask[0]=255;
    lwipx->netmask[1]=255;
    lwipx->netmask[2]=255;
    lwipx->netmask[3]=0;
    //Ĭ������:192.168.1.1
    lwipx->gateway[0]=192;
    lwipx->gateway[1]=168;
    lwipx->gateway[2]=2;
    lwipx->gateway[3]=1;
    lwipx->dhcpstatus=0;//û��DHCP
}

//LWIP��ʼ��(LWIP������ʱ��ʹ��)
//����ֵ:0,�ɹ�
//      1,�ڴ����
//      2,DM9000��ʼ��ʧ��
//      3,�������ʧ��.
u8 lwip_comm_init(void)
{
	
    OS_CPU_SR cpu_sr;

    struct netif *Netif_Init_Flag;      //����netif_add()����ʱ�ķ���ֵ,�����ж������ʼ���Ƿ�ɹ�
    struct ip_addr ipaddr;              //ip��ַ
    struct ip_addr netmask;             //��������
    struct ip_addr gw;                  //Ĭ������

    if(lwip_comm_mem_malloc())
        return 1;                       //�ڴ�����ʧ��

    sem_enc28j60input 	= OSSemCreate(0);           	//�������ݽ����ź���,������DM9000��ʼ��֮ǰ����
    sem_enc28j60lock 	= OSSemCreate(1);    	//���������ź���,��ߵ����ȼ�2

    lwip_comm_default_ip_set(&lwipdev);         //����Ĭ��IP����Ϣ

    if(ENC28J60_Init(lwipdev.mac))
	{
		CC_DEBUGF(CC_DBG_ON | CC_DBG_LEVEL_WARNING, "enc28j60 init failed\n");
        return 2;                       //��ʼ�� ENC28J60
	}

	CC_DEBUGF(CC_DBG_ON | CC_DBG_LEVEL_WARNING, "enc28j60 init ok\n");
	
    tcpip_init(NULL,NULL);              //��ʼ��tcp ip�ں�,�ú�������ᴴ��tcpip_thread�ں�����

#if LWIP_DHCP                           //ʹ�ö�̬IP
    ipaddr.addr = 0;
    netmask.addr = 0;
    gw.addr = 0;
#else
    IP4_ADDR(&ipaddr,lwipdev.ip[0],lwipdev.ip[1],lwipdev.ip[2],lwipdev.ip[3]);
    IP4_ADDR(&netmask,lwipdev.netmask[0],lwipdev.netmask[1] ,lwipdev.netmask[2],lwipdev.netmask[3]);
    IP4_ADDR(&gw,lwipdev.gateway[0],lwipdev.gateway[1],lwipdev.gateway[2],lwipdev.gateway[3]);

#endif

    Netif_Init_Flag=netif_add(&lwip_netif, &ipaddr, &netmask, &gw, NULL, &ethernetif_init, &tcpip_input);//�������б������һ������
    if(Netif_Init_Flag != NULL)         //������ӳɹ���,����netifΪĬ��ֵ,���Ҵ�netif����
    {
        netif_set_default(&lwip_netif); //����netifΪĬ������
        netif_set_up(&lwip_netif);      //��netif����
    }

    OS_ENTER_CRITICAL();        //�����ٽ���
    OSTaskCreate(lwip_netcard_input_task, (void*)0, (OS_STK*)&LWIP_ENC28J60_INPUT_TASK_STK[LWIP_DM9000_INPUT_TASK_SIZE-1], LWIP_DM9000_INPUT_TASK_PRIO);       //��̫�����ݽ�������
    OS_EXIT_CRITICAL();
	//�˳��ٽ���
#if LWIP_DHCP
    lwip_comm_dhcp_creat();     //����DHCP����
#endif

    return 0;//����OK.
    
}
//���ʹ����DHCP
#if LWIP_DHCP
//����DHCP����
void lwip_comm_dhcp_creat(void)
{
    OS_CPU_SR cpu_sr;
    OS_ENTER_CRITICAL();  //�����ٽ���
    OSTaskCreate(lwip_dhcp_task,(void*)0,(OS_STK*)&LWIP_DHCP_TASK_STK[LWIP_DHCP_STK_SIZE-1],LWIP_DHCP_TASK_PRIO);//����DHCP����
    OS_EXIT_CRITICAL();  //�˳��ٽ���
}
//ɾ��DHCP����
void lwip_comm_dhcp_delete(void)
{
    dhcp_stop(&lwip_netif);         //�ر�DHCP
    OSTaskDel(LWIP_DHCP_TASK_PRIO); //ɾ��DHCP����
}
//DHCP��������
void lwip_dhcp_task(void *pdata)
{
    u32 ip=0,netmask=0,gw=0;
    dhcp_start(&lwip_netif);//����DHCP
    lwipdev.dhcpstatus=0;   //����DHCP
    while(1)
    {

        ip=lwip_netif.ip_addr.addr;     //��ȡ��IP��ַ
        netmask=lwip_netif.netmask.addr;//��ȡ��������
        gw=lwip_netif.gw.addr;          //��ȡĬ������
        if(ip!=0)                       //����ȷ��ȡ��IP��ַ��ʱ��
        {
            lwipdev.dhcpstatus=2;   //DHCP�ɹ�

            //������ͨ��DHCP��ȡ����IP��ַ
            lwipdev.ip[3]=(uint8_t)(ip>>24);
            lwipdev.ip[2]=(uint8_t)(ip>>16);
            lwipdev.ip[1]=(uint8_t)(ip>>8);
            lwipdev.ip[0]=(uint8_t)(ip);

            //����ͨ��DHCP��ȡ�������������ַ
            lwipdev.netmask[3]=(uint8_t)(netmask>>24);
            lwipdev.netmask[2]=(uint8_t)(netmask>>16);
            lwipdev.netmask[1]=(uint8_t)(netmask>>8);
            lwipdev.netmask[0]=(uint8_t)(netmask);

            //������ͨ��DHCP��ȡ����Ĭ������
            lwipdev.gateway[3]=(uint8_t)(gw>>24);
            lwipdev.gateway[2]=(uint8_t)(gw>>16);
            lwipdev.gateway[1]=(uint8_t)(gw>>8);
            lwipdev.gateway[0]=(uint8_t)(gw);

            break;
        }
        else if(lwip_netif.dhcp->tries>LWIP_MAX_DHCP_TRIES)  //ͨ��DHCP�����ȡIP��ַʧ��,�ҳ�������Դ���
        {
            lwipdev.dhcpstatus=0XFF;//DHCPʧ��.
            //ʹ�þ�̬IP��ַ
            IP4_ADDR(&(lwip_netif.ip_addr),lwipdev.ip[0],lwipdev.ip[1],lwipdev.ip[2],lwipdev.ip[3]);
            IP4_ADDR(&(lwip_netif.netmask),lwipdev.netmask[0],lwipdev.netmask[1],lwipdev.netmask[2],lwipdev.netmask[3]);
            IP4_ADDR(&(lwip_netif.gw),lwipdev.gateway[0],lwipdev.gateway[1],lwipdev.gateway[2],lwipdev.gateway[3]);

            break;
        }
        delay_ms(250); //��ʱ250ms
    }
    lwip_comm_dhcp_delete();//ɾ��DHCP����
}
#endif

