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
u16 count = 0;
char tempStr[10];//��ص�ѹ�ַ���
u16 volatile NTC_temp;//�ֱ��¶�
u16 volatile T12_temp;//����ͷ�¶�
int main()
{
	KEY_Init();//��ʼ������GPIO
	delay_init();//��ʼ����ʱ����
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); //����NVIC�жϷ���2��2λ��ռ���ȼ���2λ�����ȼ�
	usart_init(115200);//��ʼ������1��������Ϊ115200
	TIM3_Init(19999,71);//1MHz��ÿ20ms��ⰴ��һ�Σ�
	BEEPER_Init();	//BEEPER��ʼ��
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
	T12_temp = get_T12_temp();
	NTC_temp = get_NTC_temp();//��ȡ�ֱ��¶�
	while (1){
		count++;
		if(count>20)
		{
			NTC_temp = get_NTC_temp();
			get_sleepSign();
//			printf("%4.4f\r\n",T12_temp*3.3*2/4095);//����ȵ�ż�ĵ�ѹmV����Ӧ�ֶȱ�鿴�ͺ�
			count = 0;
		}
		if(nowMenuIndex==home){
			homeWindow();//��ʾ������
			OLED_Refresh_Gram();//ˢ���Դ�
		}
		if(menuEvent[0])
		{
			if(menuEvent[1]==KEY_enter)
			{
				if(sleepFlag) {sleepFlag=0; sleepCount=0;shutCount = 0;}//�̰������ر�����
				else if(nowMenuIndex == home) sleepFlag = 1;
				else {/*nothing*/}
			}
			if(shutFlag) {shutFlag=0;shutCount = 0;OLED_Display_On();}//���ⰴ����������
			lastMenuIndex = nowMenuIndex;
			menuEvent[0] = 0;
			OLED_display();
			STMFLASH_Write(FLASH_SAVE_ADDR,(u16 *)&setData,setDataSize);//д��FLASH
		}
		if(setData.sleepTime>0 && sleepCount>setData.sleepTime*6000) {sleepFlag=1;}
		if(setData.shutTime>0 && shutCount>setData.shutTime*6000) {shutFlag=1;}
		if(shutFlag)OLED_Display_Off();
		delay_ms(10); 
	}
}
