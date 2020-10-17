#include "stm32f10x.h"
#include "key.h"
#include "delay.h"
#include "usart.h"
#include "max6675.h"
#include "pid.h"
#include "adc.h"
#include "main.h"
#include "beeper.h"
#include "flash.h"
#include "menu.h"
#include "oled.h"
#include "setting.h"

extern unsigned char logo[];
u16 num = 0;
int count = 0;
float temprature;
char tempStr[10];//��ص�ѹ�ַ���

int main()
{
	KEY_Init();//��ʼ������GPIO
	delay_init();//��ʼ����ʱ����
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); //����NVIC�жϷ���2��2λ��ռ���ȼ���2λ�����ȼ�
	usart_init(115200);//��ʼ������1��������Ϊ115200
	TIM2_Init(9999,719);//1MHz,100ms����һ��NTC����ֵ
	TIM3_Init(19999,71);//1MHz��ÿ20ms��ⰴ��һ�Σ�
	BEEPER_Init();	//BEEPER��ʼ��
	DMA1_Init();	//DMA��ʼ��
	Adc_Init();		//ADC��ʼ��
	HEAT_Init();//����ͷ���ƶ˳�ʼ��
	OLED_Init();	//��ʼ��OLED
	set_Init();//��ȡ�û���������
	TIM4_Counter_Init(999,719);//��ʱ0.01s�ж�һ��
	PID_Setup();//PID��ʼ��
	
	OLED_Clear();
	OLED_DrawPointBMP(9,0,logo,110,56,1);//��ʾlogo
	OLED_Refresh_Gram();//ˢ���Դ�
	delay_ms(100);
	OLED_Fill(0,0,127,63,0);
	while (1){
		if(nowMenuIndex==home){
			homeWindow();//��ʾ������
			OLED_Refresh_Gram();//ˢ���Դ�
		}
		if(menuEvent[0])
		{
			if(sleepFlag) {sleepFlag=0; sleepCount=0;shutCount = 0;}//�̰������ر�����
			if(shutFlag) {shutFlag=0;shutCount = 0;OLED_Display_On();}//���ⰴ����������
			lastMenuIndex = nowMenuIndex;
			menuEvent[0] = 0;
			OLED_display();
			STMFLASH_Write(FLASH_SAVE_ADDR,(u16 *)&setData,setDataSize);//д��FLASH
		}
		if(setData.sleepTime>0 && sleepCount>setData.sleepTime*6000) {heatFlag = 0;sleepFlag=1;}
		if(setData.shutTime>0 && shutCount>setData.shutTime*6000) {heatFlag = 0;shutFlag=1;}
		if(shutFlag)OLED_Display_Off();
		delay_ms(10); 
	}
}
