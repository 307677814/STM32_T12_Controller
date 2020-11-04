#include "stm32f10x.h"
#include "main.h"

extern unsigned char logo[];
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
	TIM4_Counter_Init(999,71);//��ʱ1ms�ж�һ��
	PID_Setup();//PID��ʼ��
	
	OLED_Clear();
	OLED_DrawPointBMP(9,0,logo,110,56,1);//��ʾlogo
	OLED_Refresh_Gram();//ˢ���Դ�
	delay_ms(100);
	T12_temp = get_T12_temp();
	NTC_temp = get_NTC_temp();//��ȡ�ֱ��¶�
	OLED_Fill(0,0,127,63,0);
	while (1){
		sleepCheck();//����񶯿���
		if(nowTime%505==0)//1s����һ��
		{
			NTC_temp = get_NTC_temp();
			printf("ADC:%d\r\n",T12_Average);
		}
		if(nowMenuIndex==home && nowTime%101==0)//0.1s����һ��home����
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
			if(shutFlag) {shutFlag=0;shutCount = 0;OLED_Display_On();}//���ⰴ����������
			lastMenuIndex = nowMenuIndex;
			menuEvent[0] = 0;
			OLED_display();
			STMFLASH_Write(FLASH_SAVE_ADDR,(u16 *)&setData,setDataSize);//д��FLASH
		}
		if(setData.sleepTime>0 && sleepCount>setData.sleepTime*60000) {sleepFlag=1;}
		if(setData.shutTime>0 && shutCount>setData.shutTime*60000) {shutFlag=1;}
		if(shutFlag)OLED_Display_Off();
	}
}
