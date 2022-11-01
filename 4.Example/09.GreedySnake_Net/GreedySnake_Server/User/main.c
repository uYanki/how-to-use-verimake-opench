/********************************** (C) COPYRIGHT *******************************
* File Name          : main.c
* Author             : WCH
* Version            : V1.0.0
* Date               : 2022/01/18
* Description        : Main program body.
* Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
* SPDX-License-Identifier: Apache-2.0
*******************************************************************************/

#include "debug.h"
#include "WCHNET.h"
#include "string.h"
#include "lcd.h"
#include "ch32v30x_rng.h"
/*
 *@Note
Tcp Server例程，演示Tcp Server接收到客户端后。接收数据并回传
*/


__attribute__((__aligned__(4))) ETH_DMADESCTypeDef DMARxDscrTab[ETH_RXBUFNB];                        /* MAC接收描述符 ，4字节对齐*/
__attribute__((__aligned__(4))) ETH_DMADESCTypeDef DMATxDscrTab[ETH_TXBUFNB];                        /* MAC发送描述符，4字节对齐 */

__attribute__((__aligned__(4))) u8  MACRxBuf[ETH_RXBUFNB*ETH_MAX_PACKET_SIZE];                       /* MAC接收缓冲区，4字节对齐 */
__attribute__((__aligned__(4))) u8  MACTxBuf[ETH_TXBUFNB*ETH_MAX_PACKET_SIZE];                       /* MAC发送缓冲区，4字节对齐 */


__attribute__((__aligned__(4))) SOCK_INF SocketInf[WCHNET_MAX_SOCKET_NUM];                           /* Socket信息表，4字节对齐 */
const u16 MemNum[8] = {WCHNET_NUM_IPRAW,
                         WCHNET_NUM_UDP,
                         WCHNET_NUM_TCP,
                         WCHNET_NUM_TCP_LISTEN,
                         WCHNET_NUM_TCP_SEG,
                         WCHNET_NUM_IP_REASSDATA,
                         WCHNET_NUM_PBUF,
                         WCHNET_NUM_POOL_BUF
                         };
const u16 MemSize[8] = {WCHNET_MEM_ALIGN_SIZE(WCHNET_SIZE_IPRAW_PCB),
                          WCHNET_MEM_ALIGN_SIZE(WCHNET_SIZE_UDP_PCB),
                          WCHNET_MEM_ALIGN_SIZE(WCHNET_SIZE_TCP_PCB),
                          WCHNET_MEM_ALIGN_SIZE(WCHNET_SIZE_TCP_PCB_LISTEN),
                          WCHNET_MEM_ALIGN_SIZE(WCHNET_SIZE_TCP_SEG),
                          WCHNET_MEM_ALIGN_SIZE(WCHNET_SIZE_IP_REASSDATA),
                          WCHNET_MEM_ALIGN_SIZE(WCHNET_SIZE_PBUF) + WCHNET_MEM_ALIGN_SIZE(0),
                          WCHNET_MEM_ALIGN_SIZE(WCHNET_SIZE_PBUF) + WCHNET_MEM_ALIGN_SIZE(WCHNET_SIZE_POOL_BUF)
                         };
 __attribute__((__aligned__(4)))u8 Memp_Memory[WCHNET_MEMP_SIZE];
 __attribute__((__aligned__(4)))u8 Mem_Heap_Memory[WCHNET_RAM_HEAP_SIZE];
 __attribute__((__aligned__(4)))u8 Mem_ArpTable[WCHNET_RAM_ARP_TABLE_SIZE];


#define RECE_BUF_LEN                          WCHNET_TCP_MSS*2                                   /*socket接收缓冲区的长度,最小为TCP MSS*/

u8 MACAddr[6];                                                                                   /*Mac地址*/
u8 IPAddr[4] = {192,168,1,10};                                                                   /*IP地址*/
u8 GWIPAddr[4] = {192,168,1,1};                                                                  /*网关*/
u8 IPMask[4] = {255,255,255,0};                                                                  /*子网掩码*/
u8 DESIP[4] = {192,168,1,14};                                                                   /*目的IP地址*/

u8 SocketId;                                                                                     /*socket id号*/
u8 SocketRecvBuf[WCHNET_MAX_SOCKET_NUM][RECE_BUF_LEN];                                           /*socket缓冲区*/
u8 MyBuf[RECE_BUF_LEN];
u16 desport=1000;                                                                                /*目的端口号*/
u16 srcport=1000;                                                                                /*源端口号*/


#define map_row  198
#define map_col  234
#define x_min    3
#define x_max    236
#define y_min    3
#define y_max    200

#define up      1
#define down    2
#define left    3
#define right   4
#define sel     5
#define sw1     6
#define sw2     7

uint8_t key = 0;
uint8_t dir = 0;
uint64_t score =0;
u8 linkstatus =0;
uint32_t time;

typedef struct Snakes
{
    int x;
    int y;
    struct Snakes *next;
}snake;

snake *head;

struct Foods
{
    int x;
    int y;
}food;


/*********************************************************************
 * @fn      Ethernet_LED_Configuration
 *
 * @brief   set eth data and link led pin
 *
 * @return  none
 */
void Ethernet_LED_Configuration(void)
{
    GPIO_InitTypeDef  GPIO={0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);
    GPIO.GPIO_Pin = GPIO_Pin_8|GPIO_Pin_9;
    GPIO.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB,&GPIO);
    Ethernet_LED_LINKSET(1);
    Ethernet_LED_DATASET(1);
}


/*********************************************************************
 * @fn      Ethernet_LED_LINKSET
 *
 * @brief   set eth link led,setbit 0 or 1,the link led turn on or turn off
 *
 * @return  none
 */
void Ethernet_LED_LINKSET(u8 setbit)
{
     if(setbit){
         GPIO_SetBits(GPIOB, GPIO_Pin_8);
     }
     else {
         GPIO_ResetBits(GPIOB, GPIO_Pin_8);
    }
}


/*********************************************************************
 * @fn      Ethernet_LED_DATASET
 *
 * @brief   set eth data led,setbit 0 or 1,the data led turn on or turn off
 *
 * @return  none
 */
void Ethernet_LED_DATASET(u8 setbit)
{
     if(setbit){
         GPIO_SetBits(GPIOB, GPIO_Pin_9);
     }
     else {
         GPIO_ResetBits(GPIOB, GPIO_Pin_9);
    }
}

/*********************************************************************
 * @fn      mStopIfError
 *
 * @brief   check if error.
 *
 * @return  none
 */
void mStopIfError(u8 iError)
{
    if (iError == WCHNET_ERR_SUCCESS) return;                                   /* 操作成功 */
    printf("Error: %02X\r\n", (u16)iError);                                    /* 显示错误 */
}


/*********************************************************************
 * @fn      WCHNET_LibInit
 *
 * @brief   Initializes NET.
 *
 * @return  command status
 */
u8 WCHNET_LibInit(const u8 *ip,const u8 *gwip,const u8 *mask,const u8 *macaddr)
{
    u8 i;
    struct _WCH_CFG  cfg;

    cfg.RxBufSize = RX_BUF_SIZE;
    cfg.TCPMss   = WCHNET_TCP_MSS;
    cfg.HeapSize = WCH_MEM_HEAP_SIZE;
    cfg.ARPTableNum = WCHNET_NUM_ARP_TABLE;
    cfg.MiscConfig0 = WCHNET_MISC_CONFIG0;
    WCHNET_ConfigLIB(&cfg);
    i = WCHNET_Init(ip,gwip,mask,macaddr);
    return (i);
}

/*********************************************************************
 * @fn      SET_MCO
 *
 * @brief   Set ETH Clock.
 *
 * @return  none
 */
void SET_MCO(void)
{

    RCC_PLL3Cmd(DISABLE);
    RCC_PREDIV2Config(RCC_PREDIV2_Div2);
    RCC_PLL3Config(RCC_PLL3Mul_15);
    RCC_MCOConfig(RCC_MCO_PLL3CLK);
    RCC_PLL3Cmd(ENABLE);
    Delay_Ms(100);
    while(RESET == RCC_GetFlagStatus(RCC_FLAG_PLL3RDY))
    {
        Delay_Ms(500);
    }
    RCC_AHBPeriphClockCmd(RCC_APB2Periph_AFIO,ENABLE);
}


/*********************************************************************
 * @fn      TIM2_Init
 *
 * @brief   Initializes TIM2.
 *
 * @return  none
 */
void TIM2_Init( void )
{
    TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure={0};

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    TIM_TimeBaseStructure.TIM_Period = 200-1;
    TIM_TimeBaseStructure.TIM_Prescaler =7200-1;
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);
    TIM_ITConfig(TIM2, TIM_IT_Update ,ENABLE);

    TIM_Cmd(TIM2, ENABLE);
    TIM_ClearITPendingBit(TIM2, TIM_IT_Update );
    NVIC_SetPriority(TIM2_IRQn, 0x80);
    NVIC_EnableIRQ(TIM2_IRQn);
}


/*********************************************************************
 * @fn      WCHNET_CreatTcpSocket
 *
 * @brief   Creat Tcp Socket
 *
 * @return  none
 */
void WCHNET_CreatTcpSocket(void)
{
   u8 i;
   SOCK_INF TmpSocketInf;                                                       /* 创建临时socket变量 */

   memset((void *)&TmpSocketInf,0,sizeof(SOCK_INF));                            /* 库内部会将此变量复制，所以最好将临时变量先全部清零 */
   memcpy((void *)TmpSocketInf.IPAddr,DESIP,4);                                 /* 设置目的IP地址 */
   TmpSocketInf.SourPort = srcport;                                             /* 设置源端口 */
   TmpSocketInf.ProtoType = PROTO_TYPE_TCP;                                     /* 设置socekt类型 */
   TmpSocketInf.RecvStartPoint = (u32)SocketRecvBuf[SocketId];                            /* 设置接收缓冲区的接收缓冲区 */
   TmpSocketInf.RecvBufLen = RECE_BUF_LEN ;                                     /* 设置接收缓冲区的接收长度 */
   i = WCHNET_SocketCreat(&SocketId,&TmpSocketInf);                             /* 创建socket，将返回的socket索引保存在SocketId中 */
   printf("WCHNET_SocketCreat %d\r\n",SocketId);
   mStopIfError(i);                                                             /* 检查错误 */
   i = WCHNET_SocketListen(SocketId);                                          /* 开启TCP监听 */
   mStopIfError(i);
                                                                                /* 检查错误 */
}

/*********************************************************************
 * @fn      WCHNET_HandleSockInt
 *
 * @brief   Socket Interrupt Handle
 *
 * @return  none
 */
void WCHNET_HandleSockInt(u8 sockeid,u8 initstat)
{
    u32 len;

    if(initstat & SINT_STAT_RECV)                                               /* socket接收中断*/
    {
        linkstatus=1;
       len = WCHNET_SocketRecvLen(sockeid,NULL);                                /* 获取socket缓冲区数据长度  */
       printf("WCHNET_SocketRecvLen %d  %d\r\n",len,sockeid);
       WCHNET_SocketRecv(sockeid,MyBuf,&len);                                   /* 获取socket缓冲区数据 */
       WCHNET_SocketSend(sockeid,MyBuf,&len);                                   /* 演示回传数据 */
       if(strncmp(MyBuf,"up",len)==0)
       {
            key = up;
       }
       else if(strncmp(MyBuf,"down",len)==0)
       {
            key = down;
       }
       else if(strncmp(MyBuf,"left",len)==0)
       {
            key = left;
       }
       else if(strncmp(MyBuf,"right",len)==0)
       {
            key = right;
       }
    }
    if(initstat & SINT_STAT_CONNECT)                                            /* socket连接成功中断*/
    {
        linkstatus=1;
        printf("TCP Connect Success\r\n");
        WCHNET_ModifyRecvBuf(sockeid, (u32)SocketRecvBuf[sockeid], RECE_BUF_LEN);
    }
    if(initstat & SINT_STAT_DISCONNECT)                                         /* socket连接断开中断*/
    {
        linkstatus=0;
        printf("TCP Disconnect\r\n");
    }
    if(initstat & SINT_STAT_TIM_OUT)                                            /* socket连接超时中断*/
    {
        linkstatus=0;
       printf("TCP Timout\r\n");                                                /* 延时200ms，重连*/
    }
}


/*********************************************************************
 * @fn      WCHNET_HandleGlobalInt
 *
 * @brief   Global Interrupt Handle
 *
 * @return  none
 */
void WCHNET_HandleGlobalInt(void)
{
    u8 initstat;
    u16 i;
    u8 socketinit;

    initstat = WCHNET_GetGlobalInt();                                           /* 获取全局中断标志*/
    if(initstat & GINT_STAT_UNREACH)                                            /* 不可达中断 */
    {
       printf("GINT_STAT_UNREACH\r\n");
    }
   if(initstat & GINT_STAT_IP_CONFLI)                                           /* IP冲突中断 */
   {
       printf("GINT_STAT_IP_CONFLI\r\n");
   }
   if(initstat & GINT_STAT_PHY_CHANGE)                                          /* PHY状态变化中断 */
   {
       i = WCHNET_GetPHYStatus();                                               /* 获取PHY连接状态*/
       if(i&PHY_Linked_Status)
       printf("PHY Link Success\r\n");
   }
   if(initstat & GINT_STAT_SOCKET)                                              /* SocketÖÐ¶Ï */
   {
       for(i = 0; i < WCHNET_MAX_SOCKET_NUM; i ++)
       {
           socketinit = WCHNET_GetSocketInt(i);                               /* ¶ÁsocketÖÐ¶Ï²¢ÇåÁã */
           if(socketinit)WCHNET_HandleSockInt(i,socketinit);                  /* Èç¹ûÓÐÖÐ¶ÏÔòÇåÁã */
       }
   }
}


void creatsnake(){
    snake *body,*tail;
    head = (snake *)malloc(sizeof(snake));
    body=(snake *)malloc(sizeof(snake));
    tail=(snake *)malloc(sizeof(snake));

    head->x=18;
    head->y=18;
    body->x=18;
    body->y=28;
    tail->x=18;
    tail->y=38;

    head->next=body;
    body->next=tail;
    tail->next=NULL;
}

void printsnake(){
    snake *p=head;
    uint8_t flag =0;
    while(p->next!=NULL){
        if(flag == 0){
            lcd_fill((p->x)-4,(p->y)-4,(p->x)+4,(p->y)+4,RED);
            flag++;
        }
        else {
            //body
            lcd_fill((p->x)-4,(p->y)-4,(p->x)+4,(p->y)+4,BLUE);
        }
        p=p->next;
    }
    //tail
    lcd_fill((p->x)-4,(p->y)-4,(p->x)+4,(p->y)+4,GREEN);
}

void creatfood(){
    int x,y;
    int flag=0;
    while(!flag){
        flag=1;
        x=(uint8_t)(RNG_GetRandomNumber())%(map_col-4);
        food.x=x%10==0?x+8:(x/10)>0?(x/10)*10+8:(x%10)*10+8;
        y=(uint8_t)(RNG_GetRandomNumber())%(map_row-4);
        food.y=y%10==0?y+8:(y/10)>0?(y/10)*10+8:(y%10)*10+8;

        snake* judge = head;
        while (1)  //遍历排除蛇身重复
        {
            if (judge->next == NULL) break;
            if (food.x == judge->x && food.y == judge->y)
            {
                flag = 0;
            }
            judge = judge->next;
        }
    }
    lcd_fill(food.x-4, food.y-4, food.x+4, food.y+4, RED);
}

void movesnake(){
    int x=head->x;
    int y=head->y;
    snake *p,*q;

    switch (key)
    {
    case up:
        if (dir!=down) {
           y-=10;
           dir = up;
        }
        break;
    case down:
        if (dir!=up) {
           y+=10;
           dir = down;
        }
        break;
    case left:
        if (dir!=right) {
           x-=10;
           dir = left;
        }
        break;
    case right:
        if (dir!=left) {
            x+=10;
            dir = right;
        }
        break;
    default:
        break;
    }
    if(x!=head->x||y!=head->y){
        p=q=head;
        snake *newhead=(snake *)malloc(sizeof(snake)); //创建新蛇头节点
        newhead->x=x;
        newhead->y=y;
        newhead->next = head;    //将新蛇头接到老蛇头上
        head=newhead;
        lcd_fill((head->x)-4,(head->y)-4,(head->x)+4,(head->y)+4,RED);  //打印新蛇头
        lcd_fill((q->x)-4,(q->y)-4,(q->x)+4,(q->y)+4,BLUE);  //打印原来蛇头变成蛇身体
        while(p->next!=NULL){
            q=p;
            p=p->next;
        }                        //移动到蛇尾。
        lcd_fill((p->x)-4,(p->y)-4,(p->x)+4,(p->y)+4,WHITE);//删除蛇尾，并插除蛇尾。
        q->next=NULL;
        lcd_fill((q->x)-4,(q->y)-4,(q->x)+4,(q->y)+4,GREEN);//打印新蛇尾。
        free(p);
    }
}

void eatfood(){
    if (head->x == food.x && head->y == food.y)
    {
        score++;
        creatfood();
        snake* _new = (snake*)malloc(sizeof(snake));
        snake* p;
        p = head;
        while (1)
        {
            if (p->next == NULL) break;
            p = p->next;
        }
        p->next = _new;
        _new->next = NULL;
        lcd_show_num(200, 210, score, 4,24);
    }
}

int judge(){
    if(head->x<=3||head->y<=3||head->x>=x_max-4||head->y>=y_max-4){
        return 0;
    }
    snake *p=head->next;
    while(p->next!=NULL){
        if(head->x==p->x&&head->y==p->y){//检查是不是咬到蛇身
            return 0;
        }
        p=p->next;
    }
    if(head->x==p->x&&head->y==p->y){//检查是不是咬到蛇尾
            return 0;
        }
    return 1;
}

void Refresh_TIM_Init( u16 arr, u16 psc)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;

    RCC_APB1PeriphClockCmd( RCC_APB1Periph_TIM7, ENABLE );

    TIM_TimeBaseInitStructure.TIM_Period = arr;
    TIM_TimeBaseInitStructure.TIM_Prescaler = psc;
    TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Down;
    TIM_TimeBaseInit( TIM7, &TIM_TimeBaseInitStructure);
    TIM_ITConfig(TIM7, TIM_IT_Update, ENABLE);

    TIM_ARRPreloadConfig( TIM7, ENABLE );
    TIM_Cmd( TIM7, ENABLE );

    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = TIM7_IRQn ;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1; //抢占优先级
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;        //子优先级
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

void TIM7_IRQHandler(void)   __attribute__((interrupt("WCH-Interrupt-fast")));
void TIM7_IRQHandler(void)
{
    TIM_ClearFlag(TIM7, TIM_FLAG_Update);//清除标志位
    if (judge()) {
         movesnake();
         eatfood();
     }
    else {
        lcd_show_string(50, 120, 32, "GAME OVER");
    }
}

/*********************************************************************
 * @fn      main
 *
 * @brief   Main program
 *
 * @return  none
 */
int main(void)
{
    u8 i;

	Delay_Init();
	USART_Printf_Init(115200);                                              /*串口打印初始化*/
	printf("TcpServer Test\r\n");
	lcd_init();
    lcd_set_color(WHITE, BLUE);
    lcd_fill(0, 0, 239, 239, WHITE);
    /* 显示红色小块活动范围框 */
   lcd_set_color(WHITE, BLUE);
   lcd_draw_rectangle(1,1,238,202);
   lcd_draw_rectangle(2,2,237,201);
   lcd_draw_rectangle(3,3,236,200);

   lcd_set_color(WHITE, BLUE);
   lcd_show_string(1, 205, 32, "Greedy Snake");

   RCC_AHBPeriphClockCmd(RCC_AHBPeriph_RNG, ENABLE);
   RNG_Cmd(ENABLE);

   creatsnake();
   printsnake();
   creatfood();

    SET_MCO();
    TIM2_Init();

    Refresh_TIM_Init( 2500-1, (SystemCoreClock/10000)-1 );

    WCH_GetMac(MACAddr);                                                     /*获取芯片Mac地址*/
    i=WCHNET_LibInit(IPAddr,GWIPAddr,IPMask,MACAddr);                        /*以太网库初始化*/
    mStopIfError(i);
    if(i==WCHNET_ERR_SUCCESS) printf("WCHNET_LibInit Success\r\n");
    while(!(WCHNET_GetPHYStatus()&PHY_LINK_SUCCESS))                         /*等待PHY连接成功*/
     {
       Delay_Ms(100);
     }
    WCHNET_CreatTcpSocket();                                                 /*创建Tcp socket*/

	while(1)
	{
	  WCHNET_MainTask();                                                     /*以太网库主任务函数，需要循环调用*/
	  if(WCHNET_QueryGlobalInt())                                            /*查询以太网全局中断，如果有中断，调用全局中断处理函数*/
	  {
         WCHNET_HandleGlobalInt();
	  }
    }
}

