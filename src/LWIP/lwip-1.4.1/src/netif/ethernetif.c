#include "netif/ethernetif.h"
#include "enc28j60.h"
#include "lwip_comm.h"
#include "lib_mem.h"
#include "netif/etharp.h"
#include "string.h"
#include "includes.h"

//////////////////////////////////////////////////////////////////////////////////
//������ֻ��ѧϰʹ�ã�δ��������ɣ��������������κ���;
//ALIENTEK ս�������� V3
//ethernetif ����
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

extern OS_EVENT* sem_enc28j60input;     //DM9000���������ź���

static u8 s_lwip_buf[2048];

//��ethernetif_init()�������ڳ�ʼ��Ӳ��
//netif:�����ṹ��ָ��
//����ֵ:ERR_OK,����
//       ����,ʧ��
static err_t low_level_init(struct netif *netif)
{
    //INT8U err;
    netif->hwaddr_len = ETHARP_HWADDR_LEN; //����MAC��ַ����,Ϊ6���ֽ�
    //��ʼ��MAC��ַ,����ʲô��ַ���û��Լ�����,���ǲ����������������豸MAC��ַ�ظ�
    netif->hwaddr[0]=lwipdev.mac[0];
    netif->hwaddr[1]=lwipdev.mac[1];
    netif->hwaddr[2]=lwipdev.mac[2];
    netif->hwaddr[3]=lwipdev.mac[3];
    netif->hwaddr[4]=lwipdev.mac[4];
    netif->hwaddr[5]=lwipdev.mac[5];
    netif->mtu=1500; //��������䵥Ԫ,����������㲥��ARP����
    netif->flags = NETIF_FLAG_BROADCAST|NETIF_FLAG_ETHARP|NETIF_FLAG_LINK_UP;
    return ERR_OK;
}
//���ڷ������ݰ�����ײ㺯��(lwipͨ��netif->linkoutputָ��ú���)
//netif:�����ṹ��ָ��
//p:pbuf���ݽṹ��ָ��
//����ֵ:ERR_OK,��������
//       ERR_MEM,����ʧ��
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
//���ڽ������ݰ�����ײ㺯��
//neitif:�����ṹ��ָ��
//����ֵ:pbuf���ݽṹ��ָ��
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
        OSSemPend(sem_enc28j60input, 4, &_err);     //�����ź���		4*5ms timeout
        //if(_err == OS_ERR_NONE)
        {
            while(1)
            { 
                p = low_level_input(netif);   //����low_level_input������������
                if(p != NULL)
                {
                    err = netif->input(p, netif); //����netif�ṹ���е�input�ֶ�(һ������)���������ݰ�
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

//ʹ��low_level_init()��������ʼ������
//netif:�����ṹ��ָ��
//����ֵ:ERR_OK,����
//       ����,ʧ��
err_t ethernetif_init(struct netif *netif)
{
    LWIP_ASSERT("netif!=NULL",(netif!=NULL));
#if LWIP_NETIF_HOSTNAME         //LWIP_NETIF_HOSTNAME 
    netif->hostname="lwip";     //��ʼ������
#endif
    netif->name[0]=IFNAME0;     //��ʼ������netif��name�ֶ�
    netif->name[1]=IFNAME1;     //���ļ��ⶨ�����ﲻ�ù��ľ���ֵ
    netif->output=etharp_output;//IP�㷢�����ݰ�����
    netif->linkoutput=low_level_output;//ARPģ�鷢�����ݰ�����
    low_level_init(netif);      //�ײ�Ӳ����ʼ������
    return ERR_OK;
}














