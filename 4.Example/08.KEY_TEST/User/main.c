/********************************** (C) COPYRIGHT *******************************
* File Name          : main.c
* Author             : WCH
* Version            : V1.0.0
* Date               : 2021/06/06
* Description        : Main program body.
*******************************************************************************/

/*
 *@Note 
 ADC使用DMA采样例程：
 ADC通道2(PA2),规则组通道通过DMA获取 ADC连续1024次转换数据。
 
*/

#include "debug.h"

/* Global Variable */
u16 TxBuf[1024];
s16 Calibrattion_Val = 0;


void EXTI0_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void EXTI1_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void EXTI2_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void EXTI3_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void EXTI4_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void EXTI9_5_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void EXTI15_10_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));

/*******************************************************************************
* Function Name  : EXTI0_INT_INIT
* Description    : Initializes EXTI0 collection.
* Input          : None
* Return         : None
*******************************************************************************/
void EXTI_INT_INIT(void)
{
  GPIO_InitTypeDef  GPIO_InitStructure;
    EXTI_InitTypeDef EXTI_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

  RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO|RCC_APB2Periph_GPIOA|RCC_APB2Periph_GPIOD|RCC_APB2Periph_GPIOE,ENABLE);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1|GPIO_Pin_2|GPIO_Pin_3|GPIO_Pin_4|GPIO_Pin_5;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
  GPIO_Init(GPIOE, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6|GPIO_Pin_13;
      GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
      GPIO_Init(GPIOD, &GPIO_InitStructure);



  GPIO_EXTILineConfig(GPIO_PortSourceGPIOA,GPIO_PinSource0);
  GPIO_EXTILineConfig(GPIO_PortSourceGPIOE,GPIO_PinSource1);
  GPIO_EXTILineConfig(GPIO_PortSourceGPIOE,GPIO_PinSource2);
  GPIO_EXTILineConfig(GPIO_PortSourceGPIOE,GPIO_PinSource3);
  GPIO_EXTILineConfig(GPIO_PortSourceGPIOE,GPIO_PinSource4);
  GPIO_EXTILineConfig(GPIO_PortSourceGPIOE,GPIO_PinSource5);
  GPIO_EXTILineConfig(GPIO_PortSourceGPIOD,GPIO_PinSource6);
  GPIO_EXTILineConfig(GPIO_PortSourceGPIOD,GPIO_PinSource13);


  EXTI_InitStructure.EXTI_Line=EXTI_Line0|EXTI_Line1|EXTI_Line2|EXTI_Line3|EXTI_Line4|EXTI_Line5|EXTI_Line6|EXTI_Line13;
  EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
  EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  EXTI_Init(&EXTI_InitStructure);

    NVIC_InitStructure.NVIC_IRQChannel = EXTI0_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

  NVIC_InitStructure.NVIC_IRQChannel = EXTI1_IRQn;
    NVIC_Init(&NVIC_InitStructure);

  NVIC_InitStructure.NVIC_IRQChannel = EXTI2_IRQn;
    NVIC_Init(&NVIC_InitStructure);

  NVIC_InitStructure.NVIC_IRQChannel = EXTI3_IRQn;
  NVIC_Init(&NVIC_InitStructure);

  NVIC_InitStructure.NVIC_IRQChannel = EXTI4_IRQn;
    NVIC_Init(&NVIC_InitStructure);

  NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;
  NVIC_Init(&NVIC_InitStructure);

  NVIC_InitStructure.NVIC_IRQChannel = EXTI15_10_IRQn;
    NVIC_Init(&NVIC_InitStructure);
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
        printf("SystemClk:%d\r\n",SystemCoreClock);

        printf("EXTI0 Test\r\n");
        EXTI_INT_INIT();


        while(1)
      {
       Delay_Ms(1000);
//         printf("Run at main\r\n");
        }
}

/*******************************************************************************
* Function Name  : EXTI0_IRQHandler
* Description    : This function handles EXTI0 Handler.
* Input          : None
* Return         : None
*******************************************************************************/
void EXTI0_IRQHandler(void)
{
  if(EXTI_GetITStatus(EXTI_Line0)!=RESET)
  {
    printf("Wake UP !\r\n");
    EXTI_ClearITPendingBit(EXTI_Line0);     /* Clear Flag */
  }
}

/*******************************************************************************
* Function Name  : EXTI0_IRQHandler
* Description    : This function handles EXTI0 Handler.
* Input          : None
* Return         : None
*******************************************************************************/
void EXTI1_IRQHandler(void)
{
  if(EXTI_GetITStatus(EXTI_Line1)!=RESET)
  {
    printf("UP !\r\n");
    EXTI_ClearITPendingBit(EXTI_Line1);     /* Clear Flag */
  }
}

/*******************************************************************************
* Function Name  : EXTI0_IRQHandler
* Description    : This function handles EXTI0 Handler.
* Input          : None
* Return         : None
*******************************************************************************/
void EXTI2_IRQHandler(void)
{
  if(EXTI_GetITStatus(EXTI_Line2)!=RESET)
  {
    printf("DOWN !\r\n");
    EXTI_ClearITPendingBit(EXTI_Line2);     /* Clear Flag */
  }
}

void EXTI3_IRQHandler(void)
{
  if(EXTI_GetITStatus(EXTI_Line3)!=RESET)
  {
    printf("RIGHT !\r\n");
    EXTI_ClearITPendingBit(EXTI_Line3);     /* Clear Flag */
  }
}


void EXTI4_IRQHandler(void)
{
  if(EXTI_GetITStatus(EXTI_Line4)!=RESET)
  {
    printf("KEY1 !\r\n");
    EXTI_ClearITPendingBit(EXTI_Line4);     /* Clear Flag */
  }
}

void EXTI9_5_IRQHandler(void)
{
  if(EXTI_GetITStatus(EXTI_Line6)!=RESET)
  {
    printf("LEFT !\r\n");
    EXTI_ClearITPendingBit(EXTI_Line6);     /* Clear Flag */
  }else {
      if(EXTI_GetITStatus(EXTI_Line5)!=RESET)
        {
          printf("KEY2 !\r\n");
          EXTI_ClearITPendingBit(EXTI_Line5);     /* Clear Flag */
        }
}
}

void EXTI15_10_IRQHandler(void)
{
  if(EXTI_GetITStatus(EXTI_Line13)!=RESET)
  {
    printf("SEL !\r\n");
    EXTI_ClearITPendingBit(EXTI_Line13);     /* Clear Flag */
  }
}







