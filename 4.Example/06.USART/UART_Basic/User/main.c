/*******************************************************************************
* UART 收发例程（轮询）
*
* 本例程演示使用 USART2 收发数据
*
* UART1 作为调试串口
* 请在 debug.h 中设置  UART1 为 Debug 串口，如下
* #define DEBUG   DEBUG_UART1
*
* 开发环境：MRS
*******************************************************************************/

#include "debug.h"

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

	/* USART2 TX-->PA2  RX-->PA3 */
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

	USART_Cmd(USART2, ENABLE);                                        //开启UART
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

	USARTx_CFG();           /* USART INIT */
	int i = 0;
	char str[]="Loop back from USART2.\r\n";     //发送一条提示语
	while(str[i]){
	    while(USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);    //等待 上次发送 结束
	    USART_SendData(USART2, str[i]);                                 //发送数据
	    i++;
	}
	Delay_Ms(500);
	int recv;
	while(1){
	    //串口回环，把接收到的数据原样发出
	    while(USART_GetFlagStatus(USART2, USART_FLAG_RXNE) == RESET);   //等待接收数据
	    recv = USART_ReceiveData(USART2);                               //读取接收到的数据
        while(USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);    //等待 上次发送 结束
        USART_SendData(USART2, recv);                                   //发送数据

	}

}

