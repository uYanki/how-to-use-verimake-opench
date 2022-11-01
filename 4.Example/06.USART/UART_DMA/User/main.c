/*******************************************************************************
* UART DMA 回环例程
*
*
* 本例程演示 USART2 配合 DMA 使用
* 无发送缓存，使用数据地址就地发送
* 接收缓存为环形缓冲区
* DMA 收发过程中 CPU 无需介入
*
* UART1 作为调试串口
* 请在 debug.h 中设置  UART1 为 Debug 串口，如下
* #define DEBUG   DEBUG_UART1
*
* 开发环境：MRS
*******************************************************************************/

#include "debug.h"

/* Global define */
#define RXBUF_SIZE 1024     // DMA buffer size ; DMA 接收缓存的大小
#define size(a)   (sizeof(a) / sizeof(*(a)))

/* Global Variable */
u8 RxBuffer[RXBUF_SIZE]={0};	// DMA 接收缓存


/*******************************************************************************
* Function Name  : USARTx_CFG
* Description    : Initializes the USART peripheral.
* 描述	：	串口初始化
* Input          : None
* Return         : None
*******************************************************************************/
void USARTx_CFG(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	//开启时钟
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

	/* USART2 TX-->A2  RX-->A3 */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;           //RX，输入上拉
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	USART_InitStructure.USART_BaudRate = 115200;                    // 波特率
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;     // 数据位 8
	USART_InitStructure.USART_StopBits = USART_StopBits_1;          // 停止位 1
	USART_InitStructure.USART_Parity = USART_Parity_No;             // 无校验
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None; // 无硬件流控
	USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx; //使能 RX 和 TX

	USART_Init(USART2, &USART_InitStructure);
	DMA_Cmd(DMA1_Channel6, ENABLE);                                  //开启接收 DMA
	USART_Cmd(USART2, ENABLE);                                        //开启UART
}

/*******************************************************************************
* Function Name  : DMA_INIT
* Description    : Configures the DMA.
* 描述	：	DMA 初始化
* Input          : None
* Return         : None
*******************************************************************************/
void DMA_INIT(void)
{
	DMA_InitTypeDef DMA_InitStructure;
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

	// TX DMA 初始化
	DMA_DeInit(DMA1_Channel7);
	DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)(&USART2->DATAR);        // DMA 外设基址，需指向对应的外设
	DMA_InitStructure.DMA_MemoryBaseAddr = (u32)RxBuffer;                   // DMA 内存基址，指向发送缓冲区的首地址。未指定合法地址就启动 DMA 会进 hardfault
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;                     // 方向 : 外设 作为 终点，即 内存 ->  外设
	DMA_InitStructure.DMA_BufferSize = 0;                                   // 缓冲区大小,即要 DMA 发送的数据长度,目前没有数据可发
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;        // 外设地址自增，禁用
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;                 // 内存地址自增，启用
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte; // 外设数据位宽，8位(Byte)
 	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;         // 内存数据位宽，8位(Byte)
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;                           // 普通模式，发完结束，不循环发送
	DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;                 // 优先级最高
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;                            // 实际为 M2P,所以禁用 M2M
	DMA_Init(DMA1_Channel7, &DMA_InitStructure);

	// RX DMA 初始化，环形缓冲区自动接收
	DMA_DeInit(DMA1_Channel6);
	DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)(&USART2->DATAR);
	DMA_InitStructure.DMA_MemoryBaseAddr = (u32)RxBuffer;                   // 接收缓冲区
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;                      // 方向 : 外设 作为 源，即 内存 <- 外设
	DMA_InitStructure.DMA_BufferSize = RXBUF_SIZE;                          // 缓冲区长度为 RXBUF_SIZE
	DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;                         // 循环模式，构成环形缓冲区
	DMA_Init(DMA1_Channel6, &DMA_InitStructure);

}



/*******************************************************************************
* Function Name  :  uartWriteBlocking
* Description    :  send data via USART2			用USART2发送数据，阻塞式
* Input          :  char * data          data to send	要发送的数据的首地址
*                   uint16_t num         number of data	数据长度
* Return         :  RESET                USART2 busy,failed to send	发送失败
*                   SET                  send success				发送成功
*******************************************************************************/
FlagStatus uartWriteBlocking(char * data , uint16_t num)
{
    //等待上次发送完成
	while(DMA_GetCurrDataCounter(DMA1_Channel7) != 0){
	}

    DMA_ClearFlag(DMA2_FLAG_TC8);
	DMA_Cmd(DMA1_Channel7, DISABLE );           // 关 DMA 后操作
	DMA1_Channel7->MADDR = (uint32_t)data;      // 发送缓冲区为 data
	DMA_SetCurrDataCounter(DMA1_Channel7,num);  // 设置缓冲区长度
	DMA_Cmd(DMA1_Channel7, ENABLE);             // 开 DMA
	return SET;
}

/*******************************************************************************
* Function Name  :  uartWriteStrBlocking
* Description    :  send string via USART2	用USART2发送字符串，阻塞式
* Input          :  char * str          string to send
* Return         :  RESET                USART2 busy,failed to send	发送失败
*                   SET                  send success				发送成功
*******************************************************************************/
FlagStatus uartWriteStrBlocking(char * str)
{
    uint16_t num = 0;
    while(str[num])num++;           // 计算字符串长度
    return uartWriteBlocking(str,num);
}


/*******************************************************************************
* Function Name  :  uartRead
* Description    :  read some bytes from receive buffer 从接收缓冲区读出一组数据
* Input          :  char * buffer        buffer to storage the data	用来存放读出数据的地址
*                   uint16_t num         number of data to read		要读的字节数
* Return         :  int                  number of bytes read		返回实际读出的字节数
*******************************************************************************/
uint16_t rxBufferReadPos = 0;       //接收缓冲区读指针
uint32_t uartRead(char * buffer , uint16_t num)
{
    uint16_t rxBufferEnd = RXBUF_SIZE - DMA_GetCurrDataCounter(DMA1_Channel6); //计算 DMA 数据尾的位置
    uint16_t i = 0;

    if (rxBufferReadPos == rxBufferEnd){
        // 无数据，返回
        return 0;
    }

    while (rxBufferReadPos!=rxBufferEnd && i < num){
        buffer[i] = RxBuffer[rxBufferReadPos];
        i++;
        rxBufferReadPos++;
        if(rxBufferReadPos >= RXBUF_SIZE){
            // 读指针超出接收缓冲区，回零
            rxBufferReadPos = 0;
        }
    }
    return i;
}

/*******************************************************************************
* Function Name  :  uartReadByte
* Description    :  read one byte from UART buffer	从接收缓冲区读出 1 字节数据
* Input          :  None
* Return         :  char    read data				返回读出的数据(无数据也返回0)
*******************************************************************************/
char uartReadByte()
{
    char ret;
    uint16_t rxBufferEnd = RXBUF_SIZE - DMA_GetCurrDataCounter(DMA1_Channel6);//计算 DMA 数据尾的位置
    if (rxBufferReadPos == rxBufferEnd){
        // 无数据，返回
        return 0;
    }
    ret = RxBuffer[rxBufferReadPos];
    rxBufferReadPos++;
    if(rxBufferReadPos >= RXBUF_SIZE){
        // 读指针超出接收缓冲区，回零
        rxBufferReadPos = 0;
    }
    return ret;
}
/*******************************************************************************
* Function Name  :  uartAvailable
* Description    :  get number of bytes Available to read from the UART buffer	获取缓冲区中可读数据的数量
* Input          :  None
* Return         :  uint16_t    number of bytes Available to read				返回可读数据数量
*******************************************************************************/
uint16_t uartAvailable()
{
    uint16_t rxBufferEnd = RXBUF_SIZE - DMA_GetCurrDataCounter(DMA1_Channel6);//计算 DMA 数据尾的位置
    // 计算可读字节
    if (rxBufferReadPos <= rxBufferEnd){
        return rxBufferEnd - rxBufferReadPos;
    }else{
        return rxBufferEnd +RXBUF_SIZE -rxBufferReadPos;
    }
}

/*******************************************************************************
* Function Name  : main
* Description    : Main program.
* Input          : None
* Return         : None
*******************************************************************************/
int main(void)
{
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	Delay_Init();
	USART_Printf_Init(115200);
	printf("SystemClk:%d \t---- From debug : UART%d\r\n",SystemCoreClock,DEBUG);

	DMA_INIT();
	USARTx_CFG();                                                 	/* USART INIT */
	USART_DMACmd(USART2,USART_DMAReq_Tx|USART_DMAReq_Rx,ENABLE);	//开启 USART2 DMA 接收和发送
	uartWriteStrBlocking("From USART2 DMA.\r\n");
	Delay_Ms(500);
	int period = 500; // 串口回传间隔， 单位为毫秒
	while(1){
	//单字节收发
		while(!uartAvailable()); 	//等待RX数据
		//处理（把接收到的数据发送出去）
		char c = uartReadByte();	
		uartWriteBlocking(&c,1);

	//多字节收发	
		Delay_Ms(period);			//等一会儿，攒点数据；如果数据量大，接收缓冲会溢出
		int num = uartAvailable();	//获取可读字节数
		if (num > 0 ){
            char buffer[1024];	
            uartRead(buffer , num);	//把数据从缓冲区读到 buffer 里
			uartWriteBlocking(buffer , num); //把数据发送给 PC
		}
	}

}

