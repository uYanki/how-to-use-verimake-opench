/*
 *@Note

*/

#include "debug.h"
#include "AHT_10.h"
#include "lcd.h"
#include "AP3216C.h"
#include "MPU6050.h"


/*******************************************************************************
* Function Name  : main
* Description    : Main program.
* Input          : None
* Return         : None
*******************************************************************************/
int main(void)
{
	u8 i=0;
	float temperature, humidity;
	
	u16 ir, als, ps;

    short aacx,aacy,aacz;       //加速度原始数据
    short gyrox,gyroy,gyroz;    //陀螺仪原始数据
    short temp;                 //MPU-6050 温度

	Delay_Init();
	USART_Printf_Init(115200);
	printf("SystemClk:%d\r\n",SystemCoreClock);

    lcd_init();

    lcd_set_color(BLACK,WHITE);
    lcd_show_string(50, 0, 32,"openCH.io");
    lcd_set_color(BLACK,RED);
    lcd_show_string(0, 32, 16,"I2C Device Test");
    delay_ms(100);

    lcd_set_color(BLACK,WHITE);
    lcd_show_string(0, 176, 16,"AHT10");
	while(AHT10_Init())         //初始化AHT10
    {
        printf("AHT10 Error");
        lcd_set_color(BLACK,RED);
        lcd_show_string(180, 176, 16,"Error");
        Delay_Ms(200);
        lcd_show_string(180, 176, 16,"     ");
        Delay_Ms(200);
    }

    lcd_set_color(BLACK,GREEN);
    lcd_show_string(180, 176, 16,"OK");
    lcd_set_color(BLACK,WHITE);
	lcd_show_string(0, 48, 16,"AP3216C");
    while(AP3216C_Init())       //初始化AP3216C
        {
            lcd_set_color(BLACK,RED);
            lcd_show_string(180, 48, 16,"Error");
            Delay_Ms(200);
            lcd_show_string(180, 48, 16,"     ");
            Delay_Ms(200);
        }
    lcd_set_color(BLACK,GREEN);
    lcd_show_string(180, 48, 16,"OK");
    lcd_set_color(BLACK,WHITE);
    lcd_show_string(0, 112, 16,"MPU6050");
    while(MPU_Init())
    {
        lcd_set_color(BLACK,RED);
        lcd_show_string(180, 112, 16,"Error");
        Delay_Ms(200);
        lcd_show_string(180, 112, 16,"     ");
        Delay_Ms(200);
    }
    lcd_set_color(BLACK,GREEN);
    lcd_show_string(180, 112, 16,"OK");

	while(1)
	{
		printf("read AHT10 temp\r\n");
		temperature = AHT10_Read_Temperature();
		printf("read AHT10 hum\r\n");
		humidity = AHT10_Read_Humidity();
	    lcd_set_color(BLACK,WHITE);
		lcd_show_string(0, 176, 16,"AHT10");
		lcd_set_color(BLACK,GREEN);
		lcd_show_string(30, 192, 16,"Temperature : %5d", (int)(temperature));
		lcd_show_string(30, 208, 16,"Humidity    : %5d", (int)(humidity));

        printf("read AP3216\r\n");
        AP3216C_ReadData(&ir, &ps, &als);       //读取数据
        lcd_set_color(BLACK,WHITE);
        lcd_show_string(0, 48, 16,"AP3216C");
        lcd_set_color(BLACK,GREEN);
        lcd_show_string(30, 64, 16,"IR : %5d", ir);   //IR
        lcd_show_string(30, 80, 16,"PS : %5d", ps);   //PS
        lcd_show_string(30, 96, 16,"ALS: %5d", als);  //ALS

        printf("read MPU6050 TEMP start\r\n");
        temp=MPU_Get_Temperature(); //获得温度
        printf("read MPU6050 Acc start\r\n");
        MPU_Get_Accelerometer(&aacx,&aacy,&aacz);   //获得加速度原始数据
        printf("read MPU6050 GRY start\r\n");
        MPU_Get_Gyroscope(&gyrox,&gyroy,&gyroz);    //获得陀螺仪原始数据
        lcd_set_color(BLACK,WHITE);
        lcd_show_string(0, 112, 16,"MPU6050   temp : %5d", temp);
        lcd_set_color(BLACK,GREEN);
        lcd_show_string(30, 128, 16,"X : %6d", aacx);
        lcd_show_string(30, 144, 16,"Y : %6d", aacy);
        lcd_show_string(30, 160, 16,"Z : %6d", aacz);
        lcd_show_string(128, 128, 16,"GX : %6d", gyrox);
        lcd_show_string(128, 144, 16,"GY : %6d", gyroy);
        lcd_show_string(128, 160, 16,"GZ : %6d", gyroz);

	}

}
