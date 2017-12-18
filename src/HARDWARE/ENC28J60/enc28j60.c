#include <stdio.h>
#include "enc28j60.h"
#include "delay.h"

//////////////////////////////////////////////////////////////////////////////////
//ALIENTEKս��STM32������
//ENC28J60���� ����
//����ԭ��@ALIENTEK
//������̳:www.openedv.com
//�޸�����:2012/9/28
//�汾��V1.0
//////////////////////////////////////////////////////////////////////////////////

u8 g_enc28j60_mac[6] = {0x2, 0x0 , 0x0, 0x1, 0x2, 0x3};

extern OS_EVENT* sem_enc28j60input;
extern OS_EVENT* sem_enc28j60lock;

static u8 ENC28J60BANK;
static u32 NextPacketPtr;

//SPI �ٶ����ú���
//SpeedSet:
//SPI_BaudRatePrescaler_2   2��Ƶ
//SPI_BaudRatePrescaler_8   8��Ƶ
//SPI_BaudRatePrescaler_16  16��Ƶ
//SPI_BaudRatePrescaler_256 256��Ƶ

void SPI2_SetSpeed(u8 SPI_BaudRatePrescaler)
{
    assert_param(IS_SPI_BAUDRATE_PRESCALER(SPI_BaudRatePrescaler));
    SPI2->CR1&=0XFFC7;
    SPI2->CR1|=SPI_BaudRatePrescaler;   //����SPI2�ٶ�
    SPI_Cmd(SPI2,ENABLE);

}

//SPIx ��дһ���ֽ�
//TxData:Ҫд����ֽ�
//����ֵ:��ȡ�����ֽ�
u8 SPI2_ReadWriteByte(u8 TxData)
{
    u8 retry=0;
    while (SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_TXE) == RESET) //���ָ����SPI��־λ�������:���ͻ���ձ�־λ
    {
        retry++;
        if(retry>200)return 0;
    }
    SPI_I2S_SendData(SPI2, TxData); //ͨ������SPIx����һ������
    retry=0;

    while (SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_RXNE) == RESET) //���ָ����SPI��־λ�������:���ܻ���ǿձ�־λ
    {
        retry++;
        if(retry>200)return 0;
    }
    return SPI_I2S_ReceiveData(SPI2); //����ͨ��SPIx������յ�����
}


//��λENC28J60
//����SPI��ʼ��/IO��ʼ����

void ENC28J60_SPI2_Init(void)
{
    SPI_InitTypeDef  SPI_InitStructure;
    GPIO_InitTypeDef  GPIO_InitStructure;
//    EXTI_InitTypeDef EXTI_InitStructure;
//    NVIC_InitTypeDef NVIC_InitStructure;
    RCC_APB1PeriphClockCmd( RCC_APB1Periph_SPI2,  ENABLE );//SPI2ʱ��ʹ��
    RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOB|RCC_APB2Periph_GPIOD|RCC_APB2Periph_GPIOG|RCC_APB2Periph_AFIO, ENABLE );//PORTB,D,Gʱ��ʹ��


    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;                // �˿�����
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;         //�������
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;        //IO���ٶ�Ϊ50MHz
    GPIO_Init(GPIOD, &GPIO_InitStructure);                   //�����趨������ʼ��GPIOD.2
    GPIO_SetBits(GPIOD,GPIO_Pin_2);                      //PD.2����

    //�����PB12����,��Ϊ�˷�ֹNRF24L01��SPI FLASHӰ��.
    //��Ϊ���ǹ���һ��SPI��.

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;               // PB12 ����   ����
    GPIO_Init(GPIOB, &GPIO_InitStructure);                   //�����趨������ʼ��GPIOB.12
    GPIO_SetBits(GPIOB,GPIO_Pin_12);                         //PB.12����

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6|GPIO_Pin_8;        //PG6/7/8 ����  ����
    GPIO_Init(GPIOG, &GPIO_InitStructure);                      //�����趨������ʼ��//PG6/7/8
    GPIO_SetBits(GPIOG,GPIO_Pin_6|GPIO_Pin_8);                  //PG6/7/8����

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;             //PB13/14/15�����������
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);                      //��ʼ��GPIOB

    GPIO_SetBits(GPIOB,GPIO_Pin_13|GPIO_Pin_14|GPIO_Pin_15);    //PB13/14/15����
    /*
        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;                   // PG7 INT
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
        GPIO_Init(GPIOG, &GPIO_InitStructure);
        GPIO_EXTILineConfig(GPIO_PortSourceGPIOG, GPIO_PinSource7);

        EXTI_InitStructure.EXTI_Line = EXTI_Line7;
        EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
        EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
        EXTI_InitStructure.EXTI_LineCmd = ENABLE;
        EXTI_Init(&EXTI_InitStructure);

        NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;
        NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x02;
        NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x03;
        NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
        NVIC_Init(&NVIC_InitStructure);
    */
    SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;  //����SPI�������˫�������ģʽ:SPI����Ϊ˫��˫��ȫ˫��
    SPI_InitStructure.SPI_Mode = SPI_Mode_Master;       //����SPI����ģʽ:����Ϊ��SPI
    SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;       //����SPI�����ݴ�С:SPI���ͽ���8λ֡�ṹ
    SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;      //����ͬ��ʱ�ӵĿ���״̬Ϊ�͵�ƽ
    SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;    //����ͬ��ʱ�ӵĵ�һ�������أ��������½������ݱ�����
    SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;       //NSS�ź���Ӳ����NSS�ܽţ����������ʹ��SSIλ������:�ڲ�NSS�ź���SSIλ����
    SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_256;        //���岨����Ԥ��Ƶ��ֵ:������Ԥ��ƵֵΪ256
    SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;  //ָ�����ݴ����MSBλ����LSBλ��ʼ:���ݴ����MSBλ��ʼ
    SPI_InitStructure.SPI_CRCPolynomial = 7;    //CRCֵ����Ķ���ʽ
    SPI_Init(SPI2, &SPI_InitStructure);  //����SPI_InitStruct��ָ���Ĳ�����ʼ������SPIx�Ĵ���

    SPI_Cmd(SPI2, ENABLE); //ʹ��SPI����

}

void EXTI9_5_IRQHandler(void)
{

#if OS_CRITICAL_METHOD == 3
    OS_CPU_SR  cpu_sr = 0;
#endif
    OS_ENTER_CRITICAL();
    OSIntEnter();
    OS_EXIT_CRITICAL();

    if(EXTI_GetITStatus(EXTI_Line7))
    {
        EXTI_ClearITPendingBit(EXTI_Line7);

        OSSemPost(sem_enc28j60input);

        ENC28J60_Write_Op(ENC28J60_BIT_FIELD_CLR, EIR, EIR_PKTIF| EIR_TXIF);

    }

    OSIntExit();
}

static u8 enc28j60_tx_dma_buff[1500];
void enc28j60_tx_dma_init(u32 cpar, u32 cmar)
{
	DMA_InitTypeDef DMA_InitStructure;
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);	//ʹ��DMA����
	
	DMA_DeInit(DMA1_Channel5);   //��DMA��ͨ��1�Ĵ�������Ϊȱʡֵ

	DMA_InitStructure.DMA_PeripheralBaseAddr = cpar;  //DMA�������ַ
	DMA_InitStructure.DMA_MemoryBaseAddr = cmar;  //DMA�ڴ����ַ
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;  //���ݴ��䷽�򣬴��ڴ��ȡ���͵�����
	DMA_InitStructure.DMA_BufferSize = 0;  //DMAͨ����DMA����Ĵ�С
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;  //�����ַ�Ĵ�������
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;  //�ڴ��ַ�Ĵ�������
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;  //���ݿ��Ϊ8λ
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte; //���ݿ��Ϊ8λ
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;  //����������ģʽ
	DMA_InitStructure.DMA_Priority = DMA_Priority_Medium; //DMAͨ�� xӵ�������ȼ� 
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;  //DMAͨ��xû������Ϊ�ڴ浽�ڴ洫��
	DMA_Init(DMA1_Channel5, &DMA_InitStructure);  //����DMA_InitStruct��ָ���Ĳ�����ʼ��DMA��ͨ��USART1_Tx_DMA_Channel����ʶ�ļĴ���
}

//����һ��DMA����
void enc28j60_tx_dma_enable(u16 cndtr)
{ 
	DMA_Cmd(DMA1_Channel5, DISABLE );  //�ر� ��ָʾ��ͨ��      
 	DMA_SetCurrDataCounter(DMA1_Channel5, cndtr);//DMAͨ����DMA����Ĵ�С
 	DMA_Cmd(DMA1_Channel5, ENABLE);  //ʹ�� ��ָʾ��ͨ�� 
}

void ENC28J60_Reset(void)
{

    ENC28J60_SPI2_Init();//SPI2��ʼ��
    enc28j60_tx_dma_init((u32)&SPI2->DR, (u32)enc28j60_tx_dma_buff);
    SPI2_SetSpeed(SPI_BaudRatePrescaler_4); //SPI2 SCKƵ��Ϊ36M/4=9Mhz
    ENC28J60_RST=0;         //��λENC28J60
    delay_ms(10);
    ENC28J60_RST=1;         //��λ����
    delay_ms(10);
	
	
}
//��ȡENC28J60�Ĵ���(��������)
//op��������
//addr:�Ĵ�����ַ/����
//����ֵ:����������
u8 ENC28J60_Read_Op(u8 op,u8 addr)
{
    u8 dat=0;
    ENC28J60_CS=0;
    dat=op|(addr&ADDR_MASK);
    SPI2_ReadWriteByte(dat);
    dat=SPI2_ReadWriteByte(0xFF);

    //����Ƕ�ȡMAC/MII�Ĵ���,��ڶ��ζ��������ݲ�����ȷ��,���ֲ�29ҳ
    if(addr&0x80)
        dat=SPI2_ReadWriteByte(0xFF);

    ENC28J60_CS=1;
    return dat;
}
//��ȡENC28J60�Ĵ���(��������)
//op��������
//addr:�Ĵ�����ַ
//data:����
void ENC28J60_Write_Op(u8 op,u8 addr,u8 data)
{
    u8 dat = 0;
    ENC28J60_CS=0;
    dat=op|(addr&ADDR_MASK);
    SPI2_ReadWriteByte(dat);
    SPI2_ReadWriteByte(data);
    ENC28J60_CS=1;
}
//��ȡENC28J60���ջ�������
//len:Ҫ��ȡ�����ݳ���
//data:������ݻ�����(ĩβ�Զ���ӽ�����)
void ENC28J60_Read_Buf(u32 len,u8* data)
{
    ENC28J60_CS=0;
    SPI2_ReadWriteByte(ENC28J60_READ_BUF_MEM);
    while(len)
    {
        len--;
        *data=(u8)SPI2_ReadWriteByte(0);
        data++;
    }
    *data='\0';
    ENC28J60_CS=1;
}
//��ENC28J60д���ͻ�������
//len:Ҫд������ݳ���
//data:���ݻ�����
void ENC28J60_Write_Buf(u32 len,u8* data)
{
    ENC28J60_CS=0;
    SPI2_ReadWriteByte(ENC28J60_WRITE_BUF_MEM);
    while(len)
    {
        len--;
        SPI2_ReadWriteByte(*data);
        data++;
    }
    ENC28J60_CS=1;
}
/*void ENC28J60_Write_Buf(u32 len,u8* data)
{
	u16 cnt = 0;
    ENC28J60_CS=0;
    SPI2_ReadWriteByte(ENC28J60_WRITE_BUF_MEM);

	memcpy(enc28j60_tx_dma_buff, data, len);
	SPI_I2S_DMACmd(SPI2, SPI_I2S_DMAReq_Tx, ENABLE);
	enc28j60_tx_dma_enable(len);

	do
	{
		cnt = DMA_GetCurrDataCounter(DMA1_Channel5);
	}while(cnt != 0);
			
    ENC28J60_CS=1;
}*/
//����ENC28J60�Ĵ���Bank
//ban:Ҫ���õ�bank
void ENC28J60_Set_Bank(u8 bank)
{
    if((bank&BANK_MASK)!=ENC28J60BANK)//�͵�ǰbank��һ�µ�ʱ��,������
    {
        ENC28J60_Write_Op(ENC28J60_BIT_FIELD_CLR,ECON1,(ECON1_BSEL1|ECON1_BSEL0));
        ENC28J60_Write_Op(ENC28J60_BIT_FIELD_SET,ECON1,(bank&BANK_MASK)>>5);
        ENC28J60BANK=(bank&BANK_MASK);
    }
}
//��ȡENC28J60ָ���Ĵ���
//addr:�Ĵ�����ַ
//����ֵ:����������
u8 ENC28J60_Read(u8 addr)
{
    ENC28J60_Set_Bank(addr);//����BANK

    return ENC28J60_Read_Op(ENC28J60_READ_CTRL_REG,addr);
}
//��ENC28J60ָ���Ĵ���д����
//addr:�Ĵ�����ַ
//data:Ҫд�������
void ENC28J60_Write(u8 addr,u8 data)
{
    ENC28J60_Set_Bank(addr);
    ENC28J60_Write_Op(ENC28J60_WRITE_CTRL_REG,addr,data);
}
//��ENC28J60��PHY�Ĵ���д������
//addr:�Ĵ�����ַ
//data:Ҫд�������
void ENC28J60_PHY_Write(u8 addr,u32 data)
{
    u16 retry=0;
    ENC28J60_Write(MIREGADR,addr);  //����PHY�Ĵ�����ַ
    ENC28J60_Write(MIWRL,data);     //д������
    ENC28J60_Write(MIWRH,data>>8);
    while((ENC28J60_Read(MISTAT)&MISTAT_BUSY)&&retry<0XFFF)retry++;//�ȴ�д��PHY����
}

//��ʼ��ENC28J60
//macaddr:MAC��ַ
//����ֵ:0,��ʼ���ɹ�;
//       1,��ʼ��ʧ��;
u8 ENC28J60_Init(u8* macaddr)
{
    u16 retry=0;
    ENC28J60_Reset();
    ENC28J60_Write_Op(ENC28J60_SOFT_RESET,0,ENC28J60_SOFT_RESET);//�����λ
    while(!(ENC28J60_Read(ESTAT)&ESTAT_CLKRDY)&&retry<250)//�ȴ�ʱ���ȶ�
    {
        retry++;
        delay_ms(1);
    };
    if(retry>=250)
        return 1;//ENC28J60��ʼ��ʧ��


    // do bank 0 stuff
    // initialize receive buffer
    // 16-bit transfers,must write low byte first
    // set receive buffer start address    ���ý��ջ�������ַ  8K�ֽ�����
    NextPacketPtr=RXSTART_INIT;
    // Rx start
    //���ջ�������һ��Ӳ�������ѭ��FIFO ���������ɡ�
    //�Ĵ�����ERXSTH:ERXSTL ��ERXNDH:ERXNDL ��
    //Ϊָ�룬���建���������������ڴ洢���е�λ�á�
    //ERXST��ERXNDָ����ֽھ�������FIFO�������ڡ�
    //������̫���ӿڽ��������ֽ�ʱ����Щ�ֽڱ�˳��д��
    //���ջ������� ���ǵ�д����ERXND ָ��Ĵ洢��Ԫ
    //��Ӳ�����Զ������յ���һ�ֽ�д����ERXST ָ��
    //�Ĵ洢��Ԫ�� ��˽���Ӳ��������д��FIFO ����ĵ�
    //Ԫ��
    //���ý�����ʼ�ֽ�
    ENC28J60_Write(ERXSTL,RXSTART_INIT&0xFF);
    ENC28J60_Write(ERXSTH,RXSTART_INIT>>8);
    //ERXWRPTH:ERXWRPTL �Ĵ�������Ӳ����FIFO ��
    //���ĸ�λ��д������յ����ֽڡ� ָ����ֻ���ģ��ڳ�
    //�����յ�һ�����ݰ���Ӳ�����Զ�����ָ�롣 ָ���
    //�����ж�FIFO ��ʣ��ռ�Ĵ�С  8K-1500��
    //���ý��ն�ָ���ֽ�
    ENC28J60_Write(ERXRDPTL,RXSTART_INIT&0xFF);
    ENC28J60_Write(ERXRDPTH,RXSTART_INIT>>8);
    //���ý��ս����ֽ�
    ENC28J60_Write(ERXNDL,RXSTOP_INIT&0xFF);
    ENC28J60_Write(ERXNDH,RXSTOP_INIT>>8);
    //���÷�����ʼ�ֽ�
    ENC28J60_Write(ETXSTL,TXSTART_INIT&0xFF);
    ENC28J60_Write(ETXSTH,TXSTART_INIT>>8);
    //���÷��ͽ����ֽ�
    ENC28J60_Write(ETXNDL,TXSTOP_INIT&0xFF);
    ENC28J60_Write(ETXNDH,TXSTOP_INIT>>8);
    // do bank 1 stuff,packet filter:
    // For broadcast packets we allow only ARP packtets
    // All other packets should be unicast only for our mac (MAADR)
    //
    // The pattern to match on is therefore
    // Type     ETH.DST
    // ARP      BROADCAST
    // 06 08 -- ff ff ff ff ff ff -> ip checksum for theses bytes=f7f9
    // in binary these poitions are:11 0000 0011 1111
    // This is hex 303F->EPMM0=0x3f,EPMM1=0x30
    //���չ�����
    //UCEN������������ʹ��λ
    //��ANDOR = 1 ʱ��
    //1 = Ŀ���ַ�뱾��MAC ��ַ��ƥ������ݰ���������
    //0 = ��ֹ������
    //��ANDOR = 0 ʱ��
    //1 = Ŀ���ַ�뱾��MAC ��ַƥ������ݰ��ᱻ����
    //0 = ��ֹ������
    //CRCEN���������CRC У��ʹ��λ
    //1 = ����CRC ��Ч�����ݰ�����������
    //0 = ������CRC �Ƿ���Ч
    //PMEN����ʽƥ�������ʹ��λ
    //��ANDOR = 1 ʱ��
    //1 = ���ݰ�������ϸ�ʽƥ�����������򽫱�����
    //0 = ��ֹ������
    //��ANDOR = 0 ʱ��
    //1 = ���ϸ�ʽƥ�����������ݰ���������
    //0 = ��ֹ������
    ENC28J60_Write(ERXFCON,ERXFCON_UCEN|ERXFCON_CRCEN|ERXFCON_PMEN);
    ENC28J60_Write(EPMM0,0x3f);
    ENC28J60_Write(EPMM1,0x30);
    ENC28J60_Write(EPMCSL,0xf9);
    ENC28J60_Write(EPMCSH,0xf7);
    // do bank 2 stuff
    // enable MAC receive
    //bit 0 MARXEN��MAC ����ʹ��λ
    //1 = ����MAC �������ݰ�
    //0 = ��ֹ���ݰ�����
    //bit 3 TXPAUS����ͣ����֡����ʹ��λ
    //1 = ����MAC ������ͣ����֡������ȫ˫��ģʽ�µ��������ƣ�
    //0 = ��ֹ��ͣ֡����
    //bit 2 RXPAUS����ͣ����֡����ʹ��λ
    //1 = �����յ���ͣ����֡ʱ����ֹ���ͣ�����������
    //0 = ���Խ��յ�����ͣ����֡
    ENC28J60_Write(MACON1,MACON1_MARXEN|MACON1_TXPAUS|MACON1_RXPAUS);
    // bring MAC out of reset
    //��MACON2 �е�MARST λ���㣬ʹMAC �˳���λ״̬��
    ENC28J60_Write(MACON2,0x00);
    // enable automatic padding to 60bytes and CRC operations
    //bit 7-5 PADCFG2:PACDFG0���Զ�����CRC ����λ
    //111 = ��0 ������ж�֡��64 �ֽڳ�����׷��һ����Ч��CRC
    //110 = ���Զ�����֡
    //101 = MAC �Զ�������8100h �����ֶε�VLAN Э��֡�����Զ���䵽64 �ֽڳ��������
    //��VLAN ֡���������60 �ֽڳ�������Ҫ׷��һ����Ч��CRC
    //100 = ���Զ�����֡
    //011 = ��0 ������ж�֡��64 �ֽڳ�����׷��һ����Ч��CRC
    //010 = ���Զ�����֡
    //001 = ��0 ������ж�֡��60 �ֽڳ�����׷��һ����Ч��CRC
    //000 = ���Զ�����֡
    //bit 4 TXCRCEN������CRC ʹ��λ
    //1 = ����PADCFG��Σ�MAC�����ڷ���֡��ĩβ׷��һ����Ч��CRC�� ���PADCFG�涨Ҫ
    //׷����Ч��CRC������뽫TXCRCEN ��1��
    //0 = MAC����׷��CRC�� ������4 ���ֽڣ����������Ч��CRC �򱨸������״̬������
    //bit 0 FULDPX��MAC ȫ˫��ʹ��λ
    //1 = MAC������ȫ˫��ģʽ�¡� PHCON1.PDPXMD λ������1��
    //0 = MAC�����ڰ�˫��ģʽ�¡� PHCON1.PDPXMD λ�������㡣
    ENC28J60_Write_Op(ENC28J60_BIT_FIELD_SET,MACON3,MACON3_PADCFG0|MACON3_TXCRCEN|MACON3_FRMLNEN|MACON3_FULDPX);
    // set inter-frame gap (non-back-to-back)
    //���÷Ǳ��Ա��������Ĵ����ĵ��ֽ�
    //MAIPGL�� �����Ӧ��ʹ��12h ��̸üĴ�����
    //���ʹ�ð�˫��ģʽ��Ӧ��̷Ǳ��Ա�������
    //�Ĵ����ĸ��ֽ�MAIPGH�� �����Ӧ��ʹ��0Ch
    //��̸üĴ�����
    ENC28J60_Write(MAIPGL,0x12);
    ENC28J60_Write(MAIPGH,0x0C);
    // set inter-frame gap (back-to-back)
    //���ñ��Ա��������Ĵ���MABBIPG����ʹ��
    //ȫ˫��ģʽʱ�������Ӧ��ʹ��15h ��̸üĴ�
    //������ʹ�ð�˫��ģʽʱ��ʹ��12h ���б�̡�
    ENC28J60_Write(MABBIPG,0x15);
    // Set the maximum packet size which the controller will accept
    // Do not send packets longer than MAX_FRAMELEN:
    // ���֡����  1500
    ENC28J60_Write(MAMXFLL,MAX_FRAMELEN&0xFF);
    ENC28J60_Write(MAMXFLH,MAX_FRAMELEN>>8);
    // do bank 3 stuff
    // write MAC address
    // NOTE: MAC address in ENC28J60 is byte-backward
    //����MAC��ַ
    ENC28J60_Write(MAADR5,macaddr[0]);
    ENC28J60_Write(MAADR4,macaddr[1]);
    ENC28J60_Write(MAADR3,macaddr[2]);
    ENC28J60_Write(MAADR2,macaddr[3]);
    ENC28J60_Write(MAADR1,macaddr[4]);
    ENC28J60_Write(MAADR0,macaddr[5]);
    //����PHYΪȫ˫��  LEDBΪ������
    ENC28J60_PHY_Write(PHCON1,PHCON1_PDPXMD);
    // no loopback of transmitted frames     ��ֹ����
    //HDLDIS��PHY ��˫�����ؽ�ֹλ
    //��PHCON1.PDPXMD = 1 ��PHCON1.PLOOPBK = 1 ʱ��
    //��λ�ɱ����ԡ�
    //��PHCON1.PDPXMD = 0 ��PHCON1.PLOOPBK = 0 ʱ��
    //1 = Ҫ���͵����ݽ�ͨ��˫���߽ӿڷ���
    //0 = Ҫ���͵����ݻỷ�ص�MAC ��ͨ��˫���߽ӿڷ���
    ENC28J60_PHY_Write(PHCON2,PHCON2_HDLDIS);
    // switch to bank 0
    //ECON1 �Ĵ���
    //�Ĵ���3-1 ��ʾΪECON1 �Ĵ����������ڿ���
    //ENC28J60 ����Ҫ���ܡ� ECON1 �а�������ʹ�ܡ���
    //������DMA ���ƺʹ洢��ѡ��λ��
    ENC28J60_Set_Bank(ECON1);
    // enable interrutps
    //EIE�� ��̫���ж�����Ĵ���
    //bit 7 INTIE�� ȫ��INT �ж�����λ
    //1 = �����ж��¼�����INT ����
    //0 = ��ֹ����INT ���ŵĻ������ʼ�ձ�����Ϊ�ߵ�ƽ��
    //bit 6 PKTIE�� �������ݰ��������ж�����λ
    //1 = ����������ݰ��������ж�
    //0 = ��ֹ�������ݰ��������ж�
    //ENC28J60_Write_Op(ENC28J60_BIT_FIELD_SET,EIE,EIE_INTIE|EIE_PKTIE | EIE_RXERIE);
    ENC28J60_Write_Op(ENC28J60_BIT_FIELD_CLR,EIE,EIE_INTIE|EIE_PKTIE | EIE_RXERIE);

    // enable packet reception
    //bit 2 RXEN������ʹ��λ
    //1 = ͨ����ǰ�����������ݰ�����д����ջ�����
    //0 = �������н��յ����ݰ�
    ENC28J60_Write_Op(ENC28J60_BIT_FIELD_SET,ECON1,ECON1_RXEN);
	
    if(ENC28J60_Read(MAADR5)== macaddr[0])
        return 0;//��ʼ���ɹ�
    else
        return 1;

}
//��ȡEREVID
u8 ENC28J60_Get_EREVID(void)
{
    //��EREVID ��Ҳ�洢�˰汾��Ϣ�� EREVID ��һ��ֻ����
    //�ƼĴ���������һ��5 λ��ʶ����������ʶ�����ض���Ƭ
    //�İ汾��
    return ENC28J60_Read(EREVID);
}

static u8 eir = 0;
static u8 estat = 0;
static u8 econ1 = 0;
static u8 ptk_cnt = 0;
//ͨ��ENC28J60�������ݰ�������
//len:���ݰ���С
//packet:���ݰ�
void ENC28J60_Packet_Send(u32 len,u8* packet)
{
    u8 err = OS_ERR_NONE;
    u16 retry = 0;

    OSSemPend(sem_enc28j60lock, 0, &err);
    if(err == OS_ERR_NONE)
    {
        do
        {
			eir   = ENC28J60_Read(EIR);
			estat = ENC28J60_Read(ESTAT);
			econ1 = ENC28J60_Read(ECON1);

			if(estat&(ESTAT_TXABRT | ESTAT_LATECOL))
			{
				ENC28J60_Write_Op(ENC28J60_BIT_FIELD_CLR, ESTAT, ESTAT_TXABRT | ESTAT_LATECOL);
				ENC28J60_Write_Op(ENC28J60_BIT_FIELD_CLR, EIR, EIR_TXERIF);
			}

	        retry++;
            if(retry > 200)
            {
                OSSemPost(sem_enc28j60lock);
                return ;
            }
        }while((econ1&ECON1_TXRTS | econ1&ECON1_DMAST) | (estat&ESTAT_TXABRT));


		//Errata: Transmit Logic reset
        ENC28J60_Write_Op(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_TXRST);
        ENC28J60_Write_Op(ENC28J60_BIT_FIELD_CLR, ECON1, ECON1_TXRST);

        //���÷��ͻ�������ַдָ�����
        ENC28J60_Write(EWRPTL,TXSTART_INIT&0xFF);
        ENC28J60_Write(EWRPTH,TXSTART_INIT>>8);
        //����TXNDָ�룬�Զ�Ӧ���������ݰ���С
        ENC28J60_Write(ETXNDL,(TXSTART_INIT+len)&0xFF);
        ENC28J60_Write(ETXNDH,(TXSTART_INIT+len)>>8);
        //дÿ�������ֽڣ�0x00��ʾʹ��macon3�����ã�

        ENC28J60_Write_Op(ENC28J60_BIT_FIELD_CLR, EIR, EIR_TXIF);

        ENC28J60_Write_Op(ENC28J60_WRITE_BUF_MEM,0,0x00);
        //�������ݰ������ͻ�����

        ENC28J60_Write_Buf(len,packet);
        //�������ݵ�����
        ENC28J60_Write_Op(ENC28J60_BIT_FIELD_SET,ECON1,ECON1_TXRTS);
        //��λ�����߼������⡣�μ�Rev. B4 Silicon Errata point 12.
        if((ENC28J60_Read(EIR)&EIR_TXERIF))
        {
            ENC28J60_Write_Op(ENC28J60_BIT_FIELD_CLR,ECON1,ECON1_TXRTS);
        }

        OSSemPost(sem_enc28j60lock);
    }
}


static u32 LastPacketPtr;

//�������ȡһ�����ݰ�����
//maxlen:���ݰ����������ճ���
//packet:���ݰ�������
//����ֵ:�յ������ݰ�����(�ֽ�)
u32 ENC28J60_Packet_Receive(u32 maxlen,u8* packet)
{

    u32 rxstat;
    u32 len;
    u8 err = OS_ERR_NONE;
	u16 retry = 0;
//    u8 reg = 0;
    OSSemPend(sem_enc28j60lock, 0, &err);
    if(err == OS_ERR_NONE)
    {
        
		estat = ENC28J60_Read(ESTAT);
		eir = ENC28J60_Read(EIR);
		econ1 = ENC28J60_Read(ECON1);
		
		ptk_cnt = ENC28J60_Read(EPKTCNT);
        if(ptk_cnt == 0)                        //�Ƿ��յ����ݰ�
        {
			if((!(econ1 & ECON1_RXEN) && (estat & ESTAT_CLKRDY)) || ((eir*EIR_RXERIF) && (estat & ESTAT_BUFFER)))
            {
				ENC28J60_Init(g_enc28j60_mac);
				estat = ENC28J60_Read(ESTAT);
				eir = ENC28J60_Read(EIR);
				econ1 = ENC28J60_Read(ECON1);
            }
			
			
            OSSemPost(sem_enc28j60lock);
            return 0;
        }
		
		do
        {
	        retry++;
            if(retry > 200)
            {
                OSSemPost(sem_enc28j60lock);
                return 0;
            }
        }while((econ1&ECON1_TXRTS | econ1&ECON1_DMAST) | (estat&ESTAT_RXBUSY));

        ENC28J60_Write(ERDPTL,(NextPacketPtr));             //���ý��ջ�������ָ��
        ENC28J60_Write(ERDPTH,(NextPacketPtr)>>8);

		LastPacketPtr = NextPacketPtr;
		
        NextPacketPtr=ENC28J60_Read_Op(ENC28J60_READ_BUF_MEM,0);    // ����һ������ָ��
        NextPacketPtr|=ENC28J60_Read_Op(ENC28J60_READ_BUF_MEM,0)<<8;

		if(NextPacketPtr < RXSTART_INIT || NextPacketPtr > RXSTOP_INIT)		//invalid address
		{
			estat = ENC28J60_Read(ESTAT);
            eir = ENC28J60_Read(EIR);
            econ1 = ENC28J60_Read(ECON1);
			ENC28J60_Init(g_enc28j60_mac);
			estat = ENC28J60_Read(ESTAT);
            eir = ENC28J60_Read(EIR);
            econ1 = ENC28J60_Read(ECON1);
		}
		else
		{
	        len=ENC28J60_Read_Op(ENC28J60_READ_BUF_MEM,0);              //�����ĳ���
	        len|=ENC28J60_Read_Op(ENC28J60_READ_BUF_MEM,0)<<8;

	        len-=4;                                                     //ȥ��CRC����

	        rxstat=ENC28J60_Read_Op(ENC28J60_READ_BUF_MEM,0);           //��ȡ����״̬
	        rxstat|=ENC28J60_Read_Op(ENC28J60_READ_BUF_MEM,0)<<8;

	        if (len>maxlen-1)
	            len=maxlen-1;                                           //���ƽ��ճ���

	        //���CRC�ͷ��Ŵ���
	        // ERXFCON.CRCENΪĬ������,һ�����ǲ���Ҫ���.
	        if((rxstat&0x80) == 0)
	            len = 0;                                    //��Ч
	        else
	            ENC28J60_Read_Buf(len,packet);              //�ӽ��ջ������и������ݰ�
	            
	        //RX��ָ���ƶ�����һ�����յ������ݰ��Ŀ�ʼλ��
	        //���ͷ����ǸղŶ��������ڴ�
	        ENC28J60_Write(ERXRDPTL,(NextPacketPtr));
	        ENC28J60_Write(ERXRDPTH,(NextPacketPtr)>>8);
			
			//�ݼ����ݰ���������־�����Ѿ��õ��������
        	ENC28J60_Write_Op(ENC28J60_BIT_FIELD_SET,ECON2,ECON2_PKTDEC);
		}

        OSSemPost(sem_enc28j60lock);
    }
    return(len);
}



