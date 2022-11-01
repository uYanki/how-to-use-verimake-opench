/********************************** (C) COPYRIGHT *******************************
  File Name          : main.c
  Author             : WCH
  Version            : V1.0.0
  Date               : 2021/06/06
  Description        : Main program body.
*******************************************************************************/

/*
   本样例实现从8388采集1S的录音数据，通过串口发送出来，接收数据在PC上通过对WAV头数据构建可以生成wav音频
   8388配置为8K,飞利浦格式、双通道采样，采样时left data = left ADC, right data = left ，16位数据格式。
   307配置I2S2接收8388数据，DMA
   307串口配置为串口2，DMA传输
 * */



#include "debug.h"
#include "es8388.h"
#include "lcd.h"



/* Global Variable */

#define up      1
#define down    2
#define left    3
#define right   4
#define sel     5
#define sw1     6
#define sw2     7


#define  Len    64000  //在SRAM中以16位形式存放数据长度。8K采样率，双通道，1s数据32KB

u16 * RxData  = (u16 *)0x20000500; //数据存放的首地址

void GPIO_INIT() {
  GPIO_InitTypeDef GPIO_InitTypdefStruct;

  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOD | RCC_APB2Periph_GPIOE, ENABLE);

  GPIO_InitTypdefStruct.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5;
  GPIO_InitTypdefStruct.GPIO_Mode = GPIO_Mode_IPU;
  GPIO_InitTypdefStruct.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOE, &GPIO_InitTypdefStruct);

  GPIO_InitTypdefStruct.GPIO_Pin = GPIO_Pin_0;
  GPIO_InitTypdefStruct.GPIO_Mode = GPIO_Mode_IPU;
  GPIO_InitTypdefStruct.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOA, &GPIO_InitTypdefStruct);

  GPIO_InitTypdefStruct.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_13;
  GPIO_InitTypdefStruct.GPIO_Mode = GPIO_Mode_IPU;
  GPIO_InitTypdefStruct.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOD, &GPIO_InitTypdefStruct);
}


/*******************************************************************************
  Function Name  : Basic_Key_Handle
  Description    : Basic Key Handle
  Input          : None
  Return         : 0 = no key press
                   1 = key press down
*******************************************************************************/
uint8_t Basic_Key_Handle( void )
{
  uint8_t keyval = 0;
  if ( ! GPIO_ReadInputDataBit( GPIOE, GPIO_Pin_4 ) )
  {
    Delay_Ms(20);
    if ( ! GPIO_ReadInputDataBit( GPIOE, GPIO_Pin_4 ) )
    {
      keyval = sw1;
    }
  }
  else {
    if ( ! GPIO_ReadInputDataBit( GPIOE, GPIO_Pin_5 ) )
    {
      Delay_Ms(20);
      if ( ! GPIO_ReadInputDataBit( GPIOE, GPIO_Pin_5 ) )
      {
        keyval = sw2;
      }
    }
    else {
      if ( ! GPIO_ReadInputDataBit( GPIOE, GPIO_Pin_1 ) )
      {
        Delay_Ms(20);
        if ( ! GPIO_ReadInputDataBit( GPIOE, GPIO_Pin_1 ) )
        {
          keyval = up;
        }
      }
      else {
        if ( ! GPIO_ReadInputDataBit( GPIOE, GPIO_Pin_2 ) )
        {
          Delay_Ms(20);
          if ( ! GPIO_ReadInputDataBit( GPIOE, GPIO_Pin_2 ) )
          {
            keyval = down;
          }
        }
        else {
          if ( ! GPIO_ReadInputDataBit( GPIOE, GPIO_Pin_3 ) )
          {
            Delay_Ms(20);
            if ( ! GPIO_ReadInputDataBit( GPIOE, GPIO_Pin_3 ) )
            {
              keyval = right;
            }
          }
          else {
            if ( ! GPIO_ReadInputDataBit( GPIOD, GPIO_Pin_6 ) )
            {
              Delay_Ms(20);
              if ( ! GPIO_ReadInputDataBit( GPIOD, GPIO_Pin_6 ) )
              {
                keyval = left;
              }
            }
            else {
              if ( ! GPIO_ReadInputDataBit( GPIOD, GPIO_Pin_13 ) )
              {
                Delay_Ms(20);
                if ( ! GPIO_ReadInputDataBit( GPIOD, GPIO_Pin_13 ) )
                {
                  keyval = sel;
                }
              }
            }
          }
        }
      }
    }
  }

  return keyval;
}


void LED_GPIO_Init(void)
{
  GPIO_InitTypeDef  GPIO_InitStructure;

  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOE, ENABLE);

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOE, &GPIO_InitStructure);
  GPIO_SetBits(GPIOE, GPIO_Pin_11);

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOE, &GPIO_InitStructure);
  GPIO_SetBits(GPIOE, GPIO_Pin_12);
}

//串口DMA传输8388采集的数据
void DMA_INIT(u16 bufsize)
{
  DMA_InitTypeDef DMA_InitStructure;
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

  DMA_DeInit(DMA1_Channel7);
  DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)(&USART2->DATAR);  /* USART2->DATAR:0x40004404 */
  DMA_InitStructure.DMA_MemoryBaseAddr = (u32)RxData;
  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
  DMA_InitStructure.DMA_BufferSize = bufsize;   //串口发送8位数据，传输量是存储长度两倍
  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
  DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
  DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;
  DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
  DMA_Init(DMA1_Channel7, &DMA_InitStructure);
}

void I2S2_Init_TX(void)
{

  GPIO_InitTypeDef GPIO_InitStructure;
  I2S_InitTypeDef  I2S_InitStructure;

  RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC | RCC_APB2Periph_AFIO, ENABLE);


  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_13;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOB, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOB, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOC, &GPIO_InitStructure);

  I2S_InitStructure.I2S_Mode = I2S_Mode_MasterTx;
  I2S_InitStructure.I2S_Standard = I2S_Standard_Phillips;

  I2S_InitStructure.I2S_DataFormat = I2S_DataFormat_16b;

  I2S_InitStructure.I2S_MCLKOutput = I2S_MCLKOutput_Enable;
  I2S_InitStructure.I2S_AudioFreq = I2S_AudioFreq_8k;
  I2S_InitStructure.I2S_CPOL = I2S_CPOL_Low;
  I2S_Init(SPI2, &I2S_InitStructure);

  SPI_I2S_DMACmd(SPI2, SPI_I2S_DMAReq_Tx, ENABLE);
  I2S_Cmd(SPI2, ENABLE);
}


/*******************************************************************************
  Function Name  : I2S2_Init
  Description    : Init I2S2
  Input          : None
  Return         : None
*******************************************************************************/
void I2S2_Init_RX(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;
  I2S_InitTypeDef  I2S_InitStructure;

  RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC, ENABLE);

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_13;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOB, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOB, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOC, &GPIO_InitStructure);

  I2S_InitStructure.I2S_Mode = I2S_Mode_MasterRx;
  I2S_InitStructure.I2S_Standard = I2S_Standard_Phillips;
  I2S_InitStructure.I2S_DataFormat = I2S_DataFormat_16b;

  I2S_InitStructure.I2S_MCLKOutput = I2S_MCLKOutput_Enable;
  I2S_InitStructure.I2S_AudioFreq = I2S_AudioFreq_8k;
  I2S_InitStructure.I2S_CPOL = I2S_CPOL_High;
  I2S_Init(SPI2, &I2S_InitStructure);

  SPI_I2S_DMACmd(SPI2, SPI_I2S_DMAReq_Rx, ENABLE);
  I2S_Cmd(SPI2, ENABLE); //开启I2SDMA接收8388数据  ，开启I2S就开始接收数据。
}

/*******************************************************************************
  Function Name  : DMA_Rx_Init
  Description    : Initializes the I2S2 DMA Channelx configuration.
  Input          : DMA_CHx:
                     x can be 1 to 7.
                   ppadr; Peripheral base address.
                   memadr: Memory base address.
                   bufsize: DMA channel buffer size.
  Return         : None
*******************************************************************************/
void DMA_Rx_Init( DMA_Channel_TypeDef* DMA_CHx, u32 ppadr, u32 memadr, u16 bufsize )
{
  DMA_InitTypeDef DMA_InitStructure;

  RCC_AHBPeriphClockCmd( RCC_AHBPeriph_DMA1, ENABLE );

  DMA_DeInit(DMA_CHx);

  DMA_InitStructure.DMA_PeripheralBaseAddr = ppadr;
  DMA_InitStructure.DMA_MemoryBaseAddr = memadr;
  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
  DMA_InitStructure.DMA_BufferSize = bufsize;
  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
  DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
  DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;
  DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
  DMA_Init( DMA_CHx, &DMA_InitStructure );
}

/*******************************************************************************
  Function Name  : DMA_Tx_Init
  Description    : Initializes the I2S2 DMA Channelx configuration.
  Input          : DMA_CHx:
                     x can be 1 to 7.
                   ppadr: Peripheral base address.
                   memadr: Memory base address.
                   bufsize: DMA channel buffer size.
  Return         : None
*******************************************************************************/
void DMA_Tx_Init( DMA_Channel_TypeDef* DMA_CHx, u32 ppadr, u32 memadr, u16 bufsize)
{
  DMA_InitTypeDef DMA_InitStructure;

  RCC_AHBPeriphClockCmd( RCC_AHBPeriph_DMA1, ENABLE );

  DMA_DeInit(DMA_CHx);

  DMA_InitStructure.DMA_PeripheralBaseAddr = ppadr;
  DMA_InitStructure.DMA_MemoryBaseAddr = memadr;
  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
  DMA_InitStructure.DMA_BufferSize = bufsize;
  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
  DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
  DMA_InitStructure.DMA_Priority = DMA_Priority_High;
  DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
  DMA_Init( DMA_CHx, &DMA_InitStructure );
}

void recode(void){

        ES8388_ADDA_Cfg(1,0);
        ES8388_Write_Reg(0x04, 0x00);
       GPIO_WriteBit(GPIOA,GPIO_Pin_8,1);  // 控制PA8，为1设置数据输入307
       Delay_Ms(10);
       printf("Set PA8 1\r\n");
       printf("Init I2S\r\n");
       I2S2_Init_RX();  //I2S2 端口初始化，配置I2S2 通信格式飞利浦，16位，8K  初始化结束后开启I2S2

       Delay_Ms(500); //延时500ms等待8388返回数据
       DMA_INIT(Len/2);   //初始化串口DMA

       DMA_Rx_Init( DMA1_Channel4, (u32)&SPI2->DATAR, (u32)RxData, Len/2 ); //初始化I2S2 DMA，接收8388返回数据
       DMA_Cmd( DMA1_Channel4, ENABLE );   //开启I2S2 DMA

       while( (!DMA_GetFlagStatus(DMA1_FLAG_TC4))){};    //等待一次传输完成
       printf("recode end!\r\n");
       I2S_Cmd(SPI2,DISABLE);  //关闭I2S2 ，8388停止返回数据
       DMA_DeInit(DMA1_Channel4);
       printf("recode out!\r\n");
}

void play(void){

    ES8388_ADDA_Cfg(0,1);
    ES8388_Output_Cfg(1);
    GPIO_WriteBit(GPIOA,GPIO_Pin_8,0);  // 控制PA8，为1设置数据输入307

    printf("Set PA8 0\r\n");
    Delay_Ms(10);
    printf("Init I2S\r\n");
    I2S2_Init_TX();


    DMA_Tx_Init( DMA1_Channel5, (u32)&SPI2->DATAR, (u32)RxData, Len/2  );
    Delay_Ms(10);
    DMA_Cmd( DMA1_Channel5, ENABLE );

    while( (!DMA_GetFlagStatus(DMA1_FLAG_TC5))){};

    printf("play end!\r\n");
   I2S_Cmd(SPI2,DISABLE);  //关闭I2S2 ，8388停止返回数据
   DMA_DeInit(DMA1_Channel5);
   printf("play out!\r\n");

}

void test_es8388(void) {
  static u16 PreKey = 0;
  static u16 CurKey = 0;
  PreKey = CurKey;
  CurKey = Basic_Key_Handle();

  lcd_set_color(BLACK, WHITE);
  lcd_show_string(50, 0, 32, "openCH.io");
  lcd_set_color(BLACK, RED);
  lcd_show_string(0, 32, 24, "ES8388 Test");
  lcd_set_color(BLACK, GBLUE);
  lcd_show_string(16, 64, 24, "Left:Recode");
  lcd_show_string(16, 96, 24, "Right:Play");

  if (CurKey != PreKey) {
    if (CurKey == left) {
      lcd_show_string(48, 160, 24, "Recording...    ");
      recode();
      lcd_show_string(48, 160, 24, "Recode   END    ");
      printf("recode return!\r\n");

    } else {
      if (CurKey == right) {
        lcd_show_string(48, 160, 24, " Playing...   ");
        play();
        lcd_show_string(48, 160, 24, " Play   END   ");
      }
    }
  }

}

/*******************************************************************************
  Function Name  : main
  Description    : Main program.
  Input          : None
  Return         : None
*******************************************************************************/
int main(void)
{
  int16_t key_count = 0;
  u16 Prekeyvalue = 0;
  u16 Curkeyvalue = 0;

  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
  Delay_Init();
  USART_Printf_Init(115200);
  printf("SystemClk:%d\r\n", SystemCoreClock);

  GPIO_INIT();
  LED_GPIO_Init();

  lcd_init();

  lcd_set_color(BLACK, GBLUE);

  Delay_Ms(100);

  printf("Start \r\n");

  Delay_Ms(20);

  printf("Init 8388\r\n");
  ES8388_Init();              //ES8388初始化
  ES8388_Set_Volume(22);      //设置耳机音量

  ES8388_I2S_Cfg(0,3);    //配置为飞利浦格式，16bit数据
  ES8388_ADDA_Cfg(1,1);   //关闭AD 关闭DA
  ES8388_Input_Cfg(0);    //AD选择1通道

  while (1)
  {
    Prekeyvalue = Curkeyvalue;
    Curkeyvalue = Basic_Key_Handle();

    if (Curkeyvalue != Prekeyvalue) {
      if (Curkeyvalue == sw2) {
        lcd_clear(BLACK);
        key_count++;
        if (key_count > 3) {
          key_count = 3;
        }
      } else {
        if (Curkeyvalue == sw1) {
          lcd_clear(BLACK);
          key_count--;
          if (key_count < 0) {
            key_count = 0;
          }
        }
      }
    }


    test_es8388();

  }

}







