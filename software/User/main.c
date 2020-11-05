/*
==============T20��̨������===============
���ߣ�Bibibili ����CaiZi
΢��������STM32F103C8T6
�ⲿʱ�ӣ�8MHz
===================����===================
OLED��ʾ��(7��SPI)��
	OLED_D0  -> PB13
	OLED_D1	 -> PB15
	OLED_RES -> PB15
	OLED_DC -> PB11
	OLED_CS -> PB12
��ת��������
	BM_CLK -> PB0
	BM_DT  -> PB3
	BM_SW  -> PB1
��������
	BEEPER -> PB9
������أ�
	NTC 	-> PA6
	T12_ADC -> PA4
	HEAT 	-> PA0
	SLEEP 	-> PA8
//�˰汾�Ĵ���ʵ�ֹ��ܣ�
//-T12����ͷ���¶Ȳ���
//-�������ķֶ�PID����
//-ͨ����ת�����������¶ȿ���
//-�̰���ת���������ؿɽ�������ģʽ
//-������ת���������ص����ò˵�
//-�ֱ��˶���⣨ͨ������񶯿��أ�
//-ʱ��������˯��/�ػ�ģʽ��ͨ������δʹ���ӵ�ʱ����
//-OLED�ϵ���Ϣ��ʾ
//-������
//-���û����ô洢��FLASH

*/
#include "stm32f10x.h"
#include "main.h"

extern unsigned char logo[];
char tempStr[10];//��ص�ѹ�ַ���
u16 volatile NTC_temp;//�ֱ��¶�
u16 volatile T12_temp;//����ͷ�¶�
u16 count;
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
	TIM4_Counter_Init(9999,719);//��ʱ100ms�ж�һ��
	PID_Setup();//PID��ʼ��
	
	
	OLED_Clear();
	OLED_DrawPointBMP(0,0,logo,128,64,1);//��ʾlogo
	OLED_Refresh_Gram();//ˢ���Դ�
	delay_ms(1000);
	T12_temp = get_T12_temp();
	NTC_temp = get_NTC_temp();//��ȡ�ֱ��¶�
	OLED_Fill(0,0,127,63,0);
	while (1){
		PID_Output();//����PID
		if(count%700==0)//����һ��
		{
			NTC_temp = get_NTC_temp();
			printf("ADC:%d\r\n",T12_Average);
		}
		if(nowMenuIndex==home && count%400==0)//����һ��home����
		{
			homeWindow();//��ʾ������
			OLED_Refresh_Gram();//ˢ���Դ�
		}
		if(menuEvent[0])
		{
			beeperOnce();
			if(menuEvent[1]==KEY_enter && nowMenuIndex == home)
			{
				if(sleepFlag) {sleepFlag=0; sleepCount=0;shutCount = 0;}//�̰������ر�����
				else sleepFlag = 1;
			}
			if(shutFlag) {shutFlag=0;shutCount = 0;}//���ⰴ����������
			lastMenuIndex = nowMenuIndex;
			menuEvent[0] = 0;
			OLED_display();
			STMFLASH_Write(FLASH_SAVE_ADDR,(u16 *)&setData,setDataSize);//д��FLASH
		}
		if(setData.sleepTime>0 && sleepCount>setData.sleepTime*60000) {sleepFlag=1;}
		if(setData.shutTime>0 && shutCount>setData.shutTime*60000) {shutFlag=1;}
		count++;
	}
}
