#include "netif/ethernetif.h"
#include "enc28j60.h"
#include "lwip_comm.h"
#include "lib_mem.h"
#include "netif/etharp.h"
#include "string.h"
#include "includes.h"

//////////////////////////////////////////////////////////////////////////////////
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//ALIENTEK 战舰开发板 V3
//ethernetif 代码
//正点原子@ALIENTEK
//技术论坛:www.openedv.com
//创建日期:2015/3/15
//版本：V1.0
//版权所有，盗版必究。
//Copyright(C) 广州市星翼电子科技有限公司 2009-2019
//All rights reserved
//*******************************************************************************
//修改信息
//无
//////////////////////////////////////////////////////////////////////////////////

extern OS_EVENT* sem_enc28j60input;     //DM9000接收数据信号量

static u8 s_lwip_buf[2048];

//由ethernetif_init()调用用于初始化硬件
//netif:网卡结构体指针
//返回值:ERR_OK,正常
//       其他,失败
static err_t low_level_init(struct netif *netif)
{
    //INT8U err;
    netif->hwaddr_len = ETHARP_HWADDR_LEN; //设置MAC地址长度,为6个字节
    //初始化MAC地址,设置什么地址由用户自己设置,但是不能与网络中其他设备MAC地址重复
    netif->hwaddr[0]=lwipdev.mac[0];
    netif->hwaddr[1]=lwipdev.mac[1];
    netif->hwaddr[2]=lwipdev.mac[2];
    netif->hwaddr[3]=lwipdev.mac[3];
    netif->hwaddr[4]=lwipdev.mac[4];
    netif->hwaddr[5]=lwipdev.mac[5];
    netif->mtu=1500; //最大允许传输单元,允许该网卡广播和ARP功能
    netif->flags = NETIF_FLAG_BROADCAST|NETIF_FLAG_ETHARP|NETIF_FLAG_LINK_UP;
    return ERR_OK;
}
//用于发送数据包的最底层函数(lwip通过netif->linkoutput指向该函数)
//netif:网卡结构体指针
//p:pbuf数据结构体指针
//返回值:ERR_OK,发送正常
//       ERR_MEM,发送失败
static err_t low_level_output(struct netif *netif, struct pbuf *p)
{
    struct pbuf *q;
    int send_len = 0;

#if ETH_PAD_SIZE
    pbuf_header(p, -ETH_PAD_SIZE); /* drop the padding word */
#endif

    for(q = p; q != NULL; q = q->next)
    {
        /* Send the data from the pbuf to the interface, one pbuf at a
           time. The size of the data in each pbuf is kept in the ->len
           variable. */
        //send data from(q->payload, q->len);
        memcpy((u8_t*)&s_lwip_buf[send_len], (u8_t*)q->payload, q->len);
        send_len +=q->len;
    }
    // signal that packet should be sent();
    ENC28J60_Packet_Send(send_len,s_lwip_buf);


#if ETH_PAD_SIZE
    pbuf_header(p, ETH_PAD_SIZE); /* reclaim the padding word */
#endif

    //LINK_STATS_INC(link.xmit);

    return ERR_OK;
}
//用于接收数据包的最底层函数
//neitif:网卡结构体指针
//返回值:pbuf数据结构体指针
static struct pbuf * low_level_input(struct netif *netif)
{
    struct pbuf *p = NULL, *q = NULL;
    u16_t len;
    int rev_len=0;

    /* Obtain the size of the packet and put it into the "len"
       variable. */
    len = ENC28J60_Packet_Receive(MAX_FRAMELEN, s_lwip_buf);

	if(!len)
	{
		return p;
	}

#if ETH_PAD_SIZE
    len += ETH_PAD_SIZE; /* allow room for Ethernet padding */
#endif

    /* We allocate a pbuf chain of pbufs from the pool. */
    p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);

    if (p != NULL)
    {

#if ETH_PAD_SIZE
        pbuf_header(p, -ETH_PAD_SIZE); /* drop the padding word */
#endif

        /* We iterate over the pbuf chain until we have read the entire
         * packet into the pbuf. */
        for(q = p; q != NULL; q = q->next)
        {
            /* Read enough bytes to fill this pbuf in the chain. The
             * available data in the pbuf is given by the q->len
             * variable.
             * This does not necessarily have to be a memcpy, you can also preallocate
             * pbufs for a DMA-enabled MAC and after receiving truncate it to the
             * actually received size. In this case, ensure the tot_len member of the
             * pbuf is the sum of the chained pbuf len members.
             */
            //read data into(q->payload, q->len);
            memcpy((u8_t*)q->payload, (u8_t*)&s_lwip_buf[rev_len], q->len);
            rev_len +=q->len;

        }
        // acknowledge that packet has been read();

#if ETH_PAD_SIZE
        pbuf_header(p, ETH_PAD_SIZE); /* reclaim the padding word */
#endif

        //LINK_STATS_INC(link.recv);
    }
    else
    {
        //drop packet();
        //LINK_STATS_INC(link.memerr);
        //LINK_STATS_INC(link.drop);
    }

    return p;
}

err_t ethernetif_input(struct netif *netif)
{
    INT8U _err;
    err_t err;
    struct pbuf *p;
    while(1)
    {
        OSSemPend(sem_enc28j60input, 4, &_err);     //请求信号量		4*5ms timeout
        //if(_err == OS_ERR_NONE)
        {
            while(1)
            { 
                p = low_level_input(netif);   //调用low_level_input函数接收数据
                if(p != NULL)
                {
                    err = netif->input(p, netif); //调用netif结构体中的input字段(一个函数)来处理数据包
                    if(err != ERR_OK)
                    {
                        LWIP_DEBUGF(NETIF_DEBUG,("ethernetif_input: IP input error\n"));
                        pbuf_free(p);
                        p = NULL;
                    }
                }
                else 
					break;
            }
        }
    }
}

//使用low_level_init()函数来初始化网络
//netif:网卡结构体指针
//返回值:ERR_OK,正常
//       其他,失败
err_t ethernetif_init(struct netif *netif)
{
    LWIP_ASSERT("netif!=NULL",(netif!=NULL));
#if LWIP_NETIF_HOSTNAME         //LWIP_NETIF_HOSTNAME 
    netif->hostname="lwip";     //初始化名称
#endif
    netif->name[0]=IFNAME0;     //初始化变量netif的name字段
    netif->name[1]=IFNAME1;     //在文件外定义这里不用关心具体值
    netif->output=etharp_output;//IP层发送数据包函数
    netif->linkoutput=low_level_output;//ARP模块发送数据包函数
    low_level_init(netif);      //底层硬件初始化函数
    return ERR_OK;
}














