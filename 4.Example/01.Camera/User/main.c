// 使用 DVP 接口接受摄像头图像数据，本例程中分辨率 240*160，支持 OV2640
#include "debug.h"
#include "lcd.h"
#include "ov.h"


/* DVP Work Mode */
#define RGB565_MODE   0
/* DVP Work Mode Selection */
#define DVP_Work_Mode    RGB565_MODE

UINT32  JPEG_DVPDMAaddr0 = 0x20005000;
UINT32  JPEG_DVPDMAaddr1 = 0x20005000 + OV2640_JPEG_WIDTH;

UINT32  RGB565_DVPDMAaddr0 = 0x2000A000;
UINT32  RGB565_DVPDMAaddr1 = 0x2000A000 + RGB565_COL_NUM*2;


volatile UINT32 frame_cnt = 0;
volatile UINT32 addr_cnt = 0;
volatile UINT32 href_cnt = 0;

void DVP_IRQHandler (void) __attribute__((interrupt("WCH-Interrupt-fast")));

/*******************************************************************************
* Function Name  : LCD_Reset_GPIO_Init
* Description    : Init LCD reset GPIO.
* Input          : None
* Return         : None
*******************************************************************************/
void LCD_Reset_GPIO_Init(void)
{
    GPIO_InitTypeDef  GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOD, &GPIO_InitStructure);
    GPIO_SetBits(GPIOD,GPIO_Pin_13);
}

/*******************************************************************************
* Function Name  : DMA_SRAMLCD_Init
* Description    : Init SRAMLCD DMA
* Input          : ddr: DVP data memory base addr.
* Return         : None
*******************************************************************************/
void DMA_SRAMLCD_Init(u32 ddr)
{
    DMA_InitTypeDef DMA_InitStructure;

    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA2, ENABLE);

    DMA_DeInit(DMA2_Channel5);

    DMA_InitStructure.DMA_PeripheralBaseAddr = ddr;
    DMA_InitStructure.DMA_MemoryBaseAddr = (u32)LCD_DATA;
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
    DMA_InitStructure.DMA_BufferSize = 0;
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Enable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Disable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
    DMA_InitStructure.DMA_Priority = DMA_Priority_Medium;
    DMA_InitStructure.DMA_M2M = DMA_M2M_Enable;
    DMA_Init(DMA2_Channel5, &DMA_InitStructure);
}

/*******************************************************************************
* Function Name  : DMA_SRAMLCD_Enable
* Description    : Enable SRAMLCD DMA transmission
* Input          : None
* Return         : None
*******************************************************************************/
void DMA_SRAMLCD_Enable(void)
{
    DMA_Cmd(DMA2_Channel5, DISABLE );
    DMA_SetCurrDataCounter(DMA2_Channel5,LCD_W);
    DMA_Cmd(DMA2_Channel5, ENABLE);
}

/*******************************************************************************
* Function Name  : DVP_Init
* Description    : Init DVP
* Input          : None
* Return         : None
*******************************************************************************/
void DVP_Init(void)
{
    NVIC_InitTypeDef NVIC_InitStructure;

    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DVP, ENABLE);

    DVP->CR0 &= ~RB_DVP_MSK_DAT_MOD;

#if (DVP_Work_Mode == RGB565_MODE)
    /* VSYNC、HSYNC:High level active */
    DVP->CR0 |= RB_DVP_D10_MOD | RB_DVP_V_POLAR;
    DVP->CR1 &= ~((RB_DVP_ALL_CLR)| RB_DVP_RCV_CLR);
    DVP->ROW_NUM = RGB565_ROW_NUM;               // rows
    DVP->COL_NUM = RGB565_COL_NUM*2;               // cols

    DVP->DMA_BUF0 = RGB565_DVPDMAaddr0;      //DMA addr0
    DVP->DMA_BUF1 = RGB565_DVPDMAaddr1;      //DMA addr1

#endif

    /* Set frame capture rate */
    DVP->CR1 &= ~RB_DVP_FCRC;
    DVP->CR1 |= DVP_RATE_100P;  //100%

    //Interupt Enable
    DVP->IER |= RB_DVP_IE_STP_FRM;
    DVP->IER |= RB_DVP_IE_FIFO_OV;
    DVP->IER |= RB_DVP_IE_FRM_DONE;
    DVP->IER |= RB_DVP_IE_ROW_DONE;
    DVP->IER |= RB_DVP_IE_STR_FRM;

    NVIC_InitStructure.NVIC_IRQChannel = DVP_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    DVP->CR1 |= RB_DVP_DMA_EN;  //enable DMA
    DVP->CR0 |= RB_DVP_ENABLE;  //enable DVP
}

u32 DVP_ROW_cnt=0;

/*******************************************************************************
* Function Name  : DVP_IRQHandler
* Description    : This function handles DVP exception.
* Input          : None
* Return         : None
*******************************************************************************/
void DVP_IRQHandler(void)
{

    if (DVP->IFR & RB_DVP_IF_ROW_DONE)
    {
        /* Write 0 clear 0 */
        DVP->IFR &= ~RB_DVP_IF_ROW_DONE;  //clear Interrupt

#if (DVP_Work_Mode == RGB565_MODE)
        if (addr_cnt%2)     //buf1 done
        {
            addr_cnt++;
            //Send DVP data to LCD

            DMA_Cmd(DMA2_Channel5, DISABLE );

            for (u16 i = 0;i < RGB565_COL_NUM; ++i) {
                *(u8 *)(RGB565_DVPDMAaddr0+i)=(u8)(*(u16 *)(RGB565_DVPDMAaddr0+i*2)>>2);
            }
            DMA_SetCurrDataCounter(DMA2_Channel5,RGB565_COL_NUM);
            DMA2_Channel5->PADDR = RGB565_DVPDMAaddr0;
            DMA_Cmd(DMA2_Channel5, ENABLE);

        }
        else                //buf0 done
        {
            addr_cnt++;
            //Send DVP data to LCD

            DMA_Cmd(DMA2_Channel5, DISABLE );

            for (u16 i = 0;i < RGB565_COL_NUM; ++i) {
                *(u8 *)(RGB565_DVPDMAaddr1+i)=(u8)(*(u16 *)(RGB565_DVPDMAaddr1+i*2)>>2);
            }
            DMA_SetCurrDataCounter(DMA2_Channel5,RGB565_COL_NUM);
            DMA2_Channel5->PADDR = RGB565_DVPDMAaddr1;
            DMA_Cmd(DMA2_Channel5, ENABLE);
        }

        href_cnt++;

#endif

    }

    if (DVP->IFR & RB_DVP_IF_FRM_DONE)
    {
        DVP->IFR &= ~RB_DVP_IF_FRM_DONE;  //clear Interrupt

#if (DVP_Work_Mode == RGB565_MODE)

        addr_cnt = 0;
        href_cnt = 0;

#endif

    }

    if (DVP->IFR & RB_DVP_IF_STR_FRM)
    {
        DVP->IFR &= ~RB_DVP_IF_STR_FRM;

        frame_cnt++;
    }

    if (DVP->IFR & RB_DVP_IF_STP_FRM)
    {
        DVP->IFR &= ~RB_DVP_IF_STP_FRM;

    }

    if (DVP->IFR & RB_DVP_IF_FIFO_OV)
    {
        DVP->IFR &= ~RB_DVP_IF_FIFO_OV;

        printf("FIFO OV\r\n");
    }

}


/*******************************************************************************
* Function Name  : main
* Description    : Main program.
* Input          : None PA2 - UART2
* Return         : None
*******************************************************************************/
int main(void)
{
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
	Delay_Init();
	USART_Printf_Init(115200);
	printf("SystemClk:%d\r\n",SystemCoreClock);

    lcd_init();
    printf("width:%02d height:%02d \n", LCD_W, LCD_H);
    Delay_Ms(100);
    lcd_clear(BLACK);
    lcd_show_string(60, 10, 32, "DVP_TEST");

    lcd_set_color(BLACK, RED);
    while(OV2640_Init())
    {
        lcd_show_string(10, 50, 24, "Camera Err");
        Delay_Ms(500);
        lcd_show_string(10, 50, 24, "          ");
    }

    lcd_address_set(0,80,239,239);
    printf("Camera Success\r\n");

    Delay_Ms(100);
    RGB565_Mode_Init();
    Delay_Ms(100);

#if (DVP_Work_Mode == RGB565_MODE)
    printf("RGB565_MODE\r\n");
#endif

    DMA_SRAMLCD_Init((u32)RGB565_DVPDMAaddr0);  //DMA2
    DVP_Init();

    while(1);
}



