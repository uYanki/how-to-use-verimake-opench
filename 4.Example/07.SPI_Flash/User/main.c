/********************************** (C) COPYRIGHT *******************************
* File Name          : main.c
* Author             : WCH
* Version            : V1.0.0
* Date               : 2021/06/06
* Description        : Main program body.
*******************************************************************************/

/*
 *@Note
 SPI接口操作FLASH外设例程：
 Master：SPI3_SCK(PB3)、SPI3_MISO(PB4)、SPI3_MOSI(PB5)。
 本例程演示 SPI 操作 Winbond W25Qxx SPIFLASH。
演示擦除Flash的第一扇区，Flash擦除是将数据清除为0xFF，Flash擦除可以按整片、扇区、块擦除，不可以按单个地址擦除。
 注：
 pins:
    CS   —— PA15
    DO   —— PB4(SPI3_MISO)
    WP   —— 3.3V
    DI   —— PB5(SPI3_MOSI)
    CLK  —— PB3(SPI1_SCK)
    HOLD —— 3.3V
 
*/

#include "debug.h"
#include "string.h"


/* Winbond SPIFalsh ID */
#define W25Q80  0XEF13
#define W25Q16  0XEF14
#define W25Q32  0XEF15
#define W25Q64  0XEF16
#define W25Q128 0XEF17

/* GigaDevice SPIFalsh ID */
#define GD25Q80  0XC813
#define GD25Q16  0XC814
#define GD25Q32  0XC815
#define GD25Q64  0XC816
#define GD25Q128 0XC817

/* Fudan Micro SPIFalsh ID */
#define FM25x08  0XA113
#define FM25x16  0XA114
#define FM25x32  0XA115
#define FM25x64  0XA116
#define FM25x128 0XA117


/* Winbond SPIFalsh Instruction List */  //详情见Flash芯片数据手册
#define W25X_WriteEnable          0x06
#define W25X_WriteDisable         0x04
#define W25X_ReadStatusReg      0x05
#define W25X_WriteStatusReg     0x01
#define W25X_ReadData               0x03
#define W25X_FastReadData         0x0B
#define W25X_FastReadDual         0x3B
#define W25X_PageProgram          0x02
#define W25X_BlockErase           0xD8
#define W25X_SectorErase          0x20
#define W25X_ChipErase            0xC7
#define W25X_PowerDown            0xB9
#define W25X_ReleasePowerDown   0xAB
#define W25X_DeviceID               0xAB
#define W25X_ManufactDeviceID   0x90
#define W25X_JedecDeviceID      0x9F

/* Global define */

/* Global Variable */
u8 SPI_FLASH_BUF[4096];
const u8 TEXT_Buf[]={"CH32V307 SPI FLASH 25Qxx"};
#define SIZE sizeof(TEXT_Buf)

/*******************************************************************************
* Function Name  : SPI3_ReadWriteByte
* Description    : SPI3 read or write one byte.
* Input          : TxData: write one byte data.
* Return         : Read one byte data.
*******************************************************************************/
u8 SPI3_ReadWriteByte(u8 TxData)
{
    u8 i=0;

    while (SPI_I2S_GetFlagStatus(SPI3, SPI_I2S_FLAG_TXE) == RESET)
    {
        i++;
        if(i>200)return 0;
    }

    SPI_I2S_SendData(SPI3, TxData);
    i=0;

    while (SPI_I2S_GetFlagStatus(SPI3, SPI_I2S_FLAG_RXNE) == RESET)
    {
        i++;
        if(i>200)return 0;
    }

    return SPI_I2S_ReceiveData(SPI3);
}

/*******************************************************************************
* Function Name  : SPI_Flash_Init
* Description    : Configuring the SPI for operation flash.
* Input          : None
* Return         : None
*******************************************************************************/
void SPI_Flash_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    SPI_InitTypeDef  SPI_InitStructure;

    RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOA|RCC_APB2Periph_GPIOB, ENABLE );
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI3,ENABLE);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    GPIO_SetBits(GPIOA, GPIO_Pin_15);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init( GPIOB, &GPIO_InitStructure );

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init( GPIOB, &GPIO_InitStructure );

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init( GPIOB, &GPIO_InitStructure );

    SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
    SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
    SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;
    SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;
    SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
    SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_4;
    SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
    SPI_InitStructure.SPI_CRCPolynomial = 7;
    SPI_Init(SPI3, &SPI_InitStructure);

    SPI_Cmd(SPI3, ENABLE);
}

/*******************************************************************************
* Function Name  : SPI_Flash_ReadSR
* Description    : Read W25Qxx status register.
*       ——BIT7  6   5   4   3   2   1   0
*       ——SPR   RV  TB  BP2 BP1 BP0 WEL BUSY
* Input          : None
* Return         : byte: status register value.
*******************************************************************************/
u8 SPI_Flash_ReadSR(void)
{
    u8 byte=0;

    GPIO_WriteBit(GPIOA, GPIO_Pin_15, 0);
    SPI3_ReadWriteByte(W25X_ReadStatusReg);
    byte=SPI3_ReadWriteByte(0Xff);
    GPIO_WriteBit(GPIOA, GPIO_Pin_15, 1);

    return byte;
}

/*******************************************************************************
* Function Name  : SPI_FLASH_Write_SR
* Description    : Write W25Qxx status register.
* Input          : sr:status register value.
* Return         : None
*******************************************************************************/
void SPI_FLASH_Write_SR(u8 sr)
{
    GPIO_WriteBit(GPIOA, GPIO_Pin_15, 0);
    SPI3_ReadWriteByte(W25X_WriteStatusReg);
    SPI3_ReadWriteByte(sr);
    GPIO_WriteBit(GPIOA, GPIO_Pin_15, 1);
}

/*******************************************************************************
* Function Name  : SPI_Flash_Wait_Busy
* Description    : Wait flash free.
* Input          : None
* Return         : None
*******************************************************************************/
void SPI_Flash_Wait_Busy(void)
{
    while((SPI_Flash_ReadSR()&0x01)==0x01);
}

/*******************************************************************************
* Function Name  : SPI_FLASH_Write_Enable
* Description    : Enable flash write.
* Input          : None
* Return         : None
*******************************************************************************/
void SPI_FLASH_Write_Enable(void)
{
    GPIO_WriteBit(GPIOA, GPIO_Pin_15, 0);
  SPI3_ReadWriteByte(W25X_WriteEnable);
    GPIO_WriteBit(GPIOA, GPIO_Pin_15, 1);
}

/*******************************************************************************
* Function Name  : SPI_FLASH_Write_Disable
* Description    : Disable flash write.
* Input          : None
* Return         : None
*******************************************************************************/
void SPI_FLASH_Write_Disable(void)
{
    GPIO_WriteBit(GPIOA, GPIO_Pin_15, 0);
  SPI3_ReadWriteByte(W25X_WriteDisable);
  GPIO_WriteBit(GPIOA, GPIO_Pin_15, 1);
}

/*******************************************************************************
* Function Name  : SPI_Flash_ReadID
* Description    : Read flash ID.
* Input          : None
* Return         : Temp: FLASH ID.
*******************************************************************************/
u16 SPI_Flash_ReadID(void)
{
    u16 Temp = 0;

    GPIO_WriteBit(GPIOA, GPIO_Pin_15, 0);
    SPI3_ReadWriteByte(W25X_ManufactDeviceID);
    SPI3_ReadWriteByte(0x00);
    SPI3_ReadWriteByte(0x00);
    SPI3_ReadWriteByte(0x00);
    Temp|=SPI3_ReadWriteByte(0xFF)<<8;
    Temp|=SPI3_ReadWriteByte(0xFF);
    GPIO_WriteBit(GPIOA, GPIO_Pin_15, 1);

    return Temp;
}

/*******************************************************************************
* Function Name  : SPI_Flash_Erase_Sector
* Description    : Erase one sector(4Kbyte).
* Input          : Dst_Addr:  0 —— 2047
* Return         : None
*******************************************************************************/
void SPI_Flash_Erase_Sector(u32 Dst_Addr)
{
    Dst_Addr*=4096;
  SPI_FLASH_Write_Enable();
  SPI_Flash_Wait_Busy();
  GPIO_WriteBit(GPIOA, GPIO_Pin_15, 0);
  SPI3_ReadWriteByte(W25X_SectorErase);
  SPI3_ReadWriteByte((u8)((Dst_Addr)>>16));
  SPI3_ReadWriteByte((u8)((Dst_Addr)>>8));
  SPI3_ReadWriteByte((u8)Dst_Addr);
    GPIO_WriteBit(GPIOA, GPIO_Pin_15, 1);
  SPI_Flash_Wait_Busy();
}

/*******************************************************************************
* Function Name  : SPI_Flash_Read
* Description    : Read data from flash.
* Input          : pBuffer:
*                  ReadAddr:Initial address(24bit).
*                  size: Data length.
* Return         : None
*******************************************************************************/
void SPI_Flash_Read(u8* pBuffer,u32 ReadAddr,u16 size)
{
    u16 i;

  GPIO_WriteBit(GPIOA, GPIO_Pin_15, 0);
  SPI3_ReadWriteByte(W25X_ReadData);
  SPI3_ReadWriteByte((u8)((ReadAddr)>>16));
  SPI3_ReadWriteByte((u8)((ReadAddr)>>8));
  SPI3_ReadWriteByte((u8)ReadAddr);

  for(i=0;i<size;i++)
    {
        pBuffer[i]=SPI3_ReadWriteByte(0XFF);
  }

    GPIO_WriteBit(GPIOA, GPIO_Pin_15, 1);
}

/*******************************************************************************
* Function Name  : SPI_Flash_Write_Page
* Description    : Write data by one page.
* Input          : pBuffer:
*                  WriteAddr:Initial address(24bit).
*                  size:Data length.
* Return         : None
*******************************************************************************/
void SPI_Flash_Write_Page(u8* pBuffer,u32 WriteAddr,u16 size)
{
    u16 i;

  SPI_FLASH_Write_Enable();
  GPIO_WriteBit(GPIOA, GPIO_Pin_15, 0);
  SPI3_ReadWriteByte(W25X_PageProgram);
  SPI3_ReadWriteByte((u8)((WriteAddr)>>16));
  SPI3_ReadWriteByte((u8)((WriteAddr)>>8));
  SPI3_ReadWriteByte((u8)WriteAddr);

  for(i=0;i<size;i++)
    {
        SPI3_ReadWriteByte(pBuffer[i]);
    }

  GPIO_WriteBit(GPIOA, GPIO_Pin_15, 1);
    SPI_Flash_Wait_Busy();
}

/*******************************************************************************
* Function Name  : SPI_Flash_Write_NoCheck
* Description    : Write data to flash.(need Erase)
*                  All data in address rang is 0xFF.
* Input          : pBuffer:
*                  WriteAddr: Initial address(24bit).
*                  size: Data length.
* Return         : None
*******************************************************************************/
void SPI_Flash_Write_NoCheck(u8* pBuffer,u32 WriteAddr,u16 size)
{
    u16 pageremain;

    pageremain=256-WriteAddr%256;

    if(size<=pageremain)pageremain=size;

    while(1)
    {
        SPI_Flash_Write_Page(pBuffer,WriteAddr,pageremain);

        if(size==pageremain)
        {
            break;
        }
        else
        {
            pBuffer+=pageremain;
            WriteAddr+=pageremain;
            size-=pageremain;

            if(size>256)pageremain=256;
            else pageremain=size;
        }
    }
}

/*******************************************************************************
* Function Name  : SPI_Flash_Write
* Description    : Write data to flash.(no need Erase)
* Input          : pBuffer:
*                  WriteAddr: Initial address(24bit).
*                  size: Data length.
* Return         : None
*******************************************************************************/
void SPI_Flash_Write(u8* pBuffer,u32 WriteAddr,u16 size)
{
    u32 secpos;
    u16 secoff;
    u16 secremain;
    u16 i;

    secpos=WriteAddr/4096;
    secoff=WriteAddr%4096;
    secremain=4096-secoff;

    if(size<=secremain)secremain=size;

    while(1)
    {
        SPI_Flash_Read(SPI_FLASH_BUF,secpos*4096,4096);

        for(i=0;i<secremain;i++)
        {
            if(SPI_FLASH_BUF[secoff+i]!=0XFF)break;
        }

        if(i<secremain)
        {
            SPI_Flash_Erase_Sector(secpos);

            for(i=0;i<secremain;i++)
            {
                SPI_FLASH_BUF[i+secoff]=pBuffer[i];
            }

            SPI_Flash_Write_NoCheck(SPI_FLASH_BUF,secpos*4096,4096);

        }
        else{
            SPI_Flash_Write_NoCheck(pBuffer,WriteAddr,secremain);
        }

        if(size==secremain){
            break;
        }
        else
        {
            secpos++;
            secoff=0;

          pBuffer+=secremain;
            WriteAddr+=secremain;
          size-=secremain;

            if(size>4096)
            {
                secremain=4096;
            }
            else
            {
                secremain=size;
            }
        }
    }
}

/*******************************************************************************
* Function Name  : SPI_Flash_Erase_Chip
* Description    : Erase all FLASH pages.
* Input          : None
* Return         : None
*******************************************************************************/
void SPI_Flash_Erase_Chip(void)
{
  SPI_FLASH_Write_Enable();
  SPI_Flash_Wait_Busy();
  GPIO_WriteBit(GPIOA, GPIO_Pin_15, 0);
  SPI3_ReadWriteByte(W25X_ChipErase);
  GPIO_WriteBit(GPIOA, GPIO_Pin_15, 1);
    SPI_Flash_Wait_Busy();
}

/*******************************************************************************
* Function Name  : SPI_Flash_PowerDown
* Description    : Enter power down mode.
* Input          : None
* Return         : None
*******************************************************************************/
void SPI_Flash_PowerDown(void)
{
  GPIO_WriteBit(GPIOA, GPIO_Pin_15, 0);
  SPI3_ReadWriteByte(W25X_PowerDown);
  GPIO_WriteBit(GPIOA, GPIO_Pin_15, 1);
  Delay_Us(3);
}

/*******************************************************************************
* Function Name  : SPI_Flash_WAKEUP
* Description    : Power down wake up.
* Input          : None
* Return         : None
*******************************************************************************/
void SPI_Flash_WAKEUP(void)
{
  GPIO_WriteBit(GPIOA, GPIO_Pin_15, 0);
    SPI3_ReadWriteByte(W25X_ReleasePowerDown);
  GPIO_WriteBit(GPIOA, GPIO_Pin_15, 1);
    Delay_Us(3);
}

/*******************************************************************************
* Function Name  : main
* Description    : Main program.
* Input          : None
* Return         : None
*******************************************************************************/
int main(void)
{
    u8 datap[SIZE]={0};
    u16 Flash_Model;

    Delay_Init();
    USART_Printf_Init(115200);
    printf("SystemClk:%d\r\n",SystemCoreClock);

    SPI_Flash_Init();

    Flash_Model = SPI_Flash_ReadID();           //读取芯片ID
    printf("Flash ID : 0x%X\r\n",Flash_Model);  //打印芯片ID

    switch(Flash_Model)                         //按芯片ID显示芯片型号
    {
        case W25Q80:
            printf("W25Q80 OK!\r\n");

            break;

        case W25Q16:
            printf("W25Q16 OK!\r\n");

            break;

        case W25Q32:
            printf("W25Q32 OK!\r\n");

            break;

        case W25Q64:
            printf("W25Q64 OK!\r\n");

            break;

        case W25Q128:
            printf("W25Q128 OK!\r\n");

            break;

		case GD25Q80:
            printf("GD25Q80 OK!\r\n");

            break;

        case GD25Q16:
            printf("GD25Q16 OK!\r\n");

            break;

        case GD25Q32:
            printf("GD25Q32 OK!\r\n");

            break;

        case GD25Q64:
            printf("GD25Q64 OK!\r\n");

            break;

        case GD25Q128:
            printf("GD25Q128 OK!\r\n");

            break;
        case FM25x08:
            printf("FM25x80 OK!\r\n");

            break;

        case FM25x16:
            printf("FM25x16 OK!\r\n");

            break;

        case FM25x32:
            printf("FM25x32 OK!\r\n");

            break;

        case FM25x64:
            printf("FM25x64 OK!\r\n");

            break;

        case FM25x128:
            printf("FM25x128 OK!\r\n");

            break;

        default:
            printf("Fail!\r\n");

          break;

    }
    printf("Start Erase 25Qxx....\r\n");
    SPI_Flash_Erase_Sector(0);                  //擦除芯片的第一个扇区。以0xFF填充
    printf("25Qxx Erase Finished!\r\n");

    Delay_Ms(500);
    printf("Start Read 25Qxx....\r\n");
    SPI_Flash_Read(datap,0x0,SIZE);             //读取第一个扇区的SIZE个数据，返回0xFF以验证Flash擦除是填充0xFF
    for (int i = 0; i < SIZE; ++i) {
        printf("0x%x ", datap[i] );
    }
    printf("\r\n");

    Delay_Ms(500);
    printf("Start Write 25Qxx....\r\n");
    SPI_Flash_Write((u8*)TEXT_Buf,0x0,SIZE);    //在0x0地址开始写入“CH32V307 SPI FLASH W25Qxx”字符串
    printf("25Qxx Write Finished!\r\n");

    Delay_Ms(500);
    printf("Start Read 25Qxx....\r\n");
    SPI_Flash_Read(datap,0x0,SIZE);             //读取0x0地址开始的数据，即刚刚写入的字符串，并打印出来验证。
    printf("%s\r\n", datap );

    while(1);
}

