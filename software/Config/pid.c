#include "pid.h"
#include "main.h"
#include "adc.h"
#include "usart.h"
#include "key.h"
#include "menu.h"
#include "setting.h"

#define DBGMCU_CR  (*((volatile u32 *)0xE0042004))
u16 timecount,g_bPIDRunFlag;
int uk,duk;//ukΪ��ǰ���ֵ��dukΪPID����
float aggKp,aggKi,aggKd;//������PID����
float consKp,consKi,consKd;//���ص�PID����
float e0,e1,e2;
float st,pt;
u16 sampleT=255;//��������100
volatile u32 nowTime = 0;//��������ʱ�䣬��λ0.01s
volatile u32 sleepCount = 0;
volatile u32 shutCount = 0;
//PID������ʼ��
void PID_Setup(void)
{
	aggKp = 11;//�������������õ�������
	aggKi = 0.5;//���ֲ���T/Ti������������̬���
	aggKd = 1;//΢�ֲ���Td/T������Ԥ�����ı仯��������ǰ����
	consKp=11;//���ص�PID����
	consKi=3;
	consKd=5;
}
//����PID���uk
void PID_Operation(void)
{
	pt = get_T12_temp();//��ǰ�¶�ֵ
	T12_temp = pt;
	e0=setData.setTemp-pt;
	if(e0>50)//�²�>50��ʱ�����м�����PID����
	{
		duk=aggKp*(e0-e1)+aggKp*aggKi*e0+aggKp*aggKd*(e0-2*e1+e2);
		uk=uk+duk;
	}
	else//�²�<30��ʱ�����б��ص�PID����
	{
		duk=consKp*(e0-e1)+consKp*consKi*e0+consKp*consKd*(e0-2*e1+e2);
		uk=uk+duk;
	}
	if(uk>sampleT) uk=sampleT;//��ֹ����
	if(uk<0) uk=0;
	e2=e1;
	e1=e0;
}
/* ********************************************************
* �������ƣ�PID_Output()                                                                         
* �������ܣ�PID�������                                         
* ��ڲ������ޣ��������룬U(k)��                                                 
* ���ڲ������ޣ����ƶˣ�                                                                               
******************************************************** */
void PID_Output(void)
{
	if(uk <= 0)
	{
		HEAT=0; //������
	}
	else if(sleepFlag==0&&shutFlag==0) 
	{
		HEAT = 1;//����
	}
	else HEAT=0; //������
	if(uk) uk--;                //ֻ��uk>0�����б�Ҫ����1��
//	if(timecount%10==0) printf("uk:%d,e0:%2.1f,e1:%2.1f,e2:%2.1f\r\n",uk,e0,e1,e2);
	timecount++;
	if(timecount >= sampleT)
	{
		PID_Operation();        //ÿ��0.1*255s����һ��PID���㡣
		timecount = 0;       
	}
}
//����ͷ�������ų�ʼ��-PB4
void HEAT_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);	  //ʹ��GPIOAʱ��
  
	GPIO_InitStructure.GPIO_Pin = HEAT_Pin;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(HEAT_GPIO_Port, &GPIO_InitStructure);
	GPIO_SetBits(HEAT_GPIO_Port,HEAT_Pin);//����
}
	
//��ʱ��4��ʼ����ΪPID�ṩ��ʱ
void TIM4_Counter_Init(u16 arr,u16 psc)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4,ENABLE); 
	
    TIM_TimeBaseInitStructure.TIM_Period = arr; //�Զ���װ�ؼĴ������ڵ�ֵ
	TIM_TimeBaseInitStructure.TIM_Prescaler=psc;  //Ԥ��Ƶֵ
	TIM_TimeBaseInitStructure.TIM_CounterMode=TIM_CounterMode_Up; // ���ϼ���
	TIM_TimeBaseInitStructure.TIM_ClockDivision=TIM_CKD_DIV1; //ʱ�ӷָ�Ϊ0,��Ȼʹ��72MHz
	TIM_ITConfig(TIM4, TIM_IT_Update, ENABLE);//��������ж�
	TIM_TimeBaseInit(TIM4,&TIM_TimeBaseInitStructure);
	
	
	NVIC_InitStructure.NVIC_IRQChannel = TIM4_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=0;//��ռ���ȼ�0
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;		//�����ȼ�3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQͨ��ʹ��
	NVIC_Init(&NVIC_InitStructure);	//����ָ���Ĳ�����ʼ��VIC�Ĵ���
	
	TIM_Cmd(TIM4,ENABLE);
}
//TIM4��ʱ���жϷ�����
void TIM4_IRQHandler(void)
{
	if(TIM_GetITStatus(TIM4, TIM_IT_Update)!=RESET)//���TIM4�����жϷ������
	{
		TIM_ClearITPendingBit(TIM4, TIM_IT_Update);//���TIM4�����жϱ�־
		PID_Output();//ÿ100us����һ��PID
		nowTime++;//1ms����һ�μ�ʱ��
		sleepCount++;
		shutCount++;
	}
}
//��ȡʱ���ַ���
void getClockTime(char timeStr[])
{
	u32 hour=0,min=0,sec=0;
	sec = nowTime/1000;
	hour = sec/3600;
	min = sec%3600/60;
	sec = sec-hour*3600-min*60;
	sprintf((char *)timeStr,"%02d:%02d:%02d",hour,min,sec);//���ʱ���ַ���
}

