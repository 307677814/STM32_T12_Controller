#include "pid.h"
#include "main.h"
#include "adc.h"
#include "usart.h"
#include "key.h"
#include "menu.h"

#define DBGMCU_CR  (*((volatile u32 *)0xE0042004))
u16 timecount,g_bPIDRunFlag;
int uk,duk;//ukΪ��ǰ���ֵ��dukΪPID����
float kp,ki,kd;
float e0,e1,e2;
float st,pt;
u16 sampleT=100;//��������100
volatile u32 nowTime = 0;//��������ʱ�䣬��λ0.01s
volatile u32 sleepCount = 0;
volatile u32 shutCount = 0;
//PID������ʼ��
void PID_Setup(void)
{
	ki = sampleT/60.0;//���ֲ���
	kp = 20.0;//�������������õ�������T/Ti������������̬���
	kd = 40/sampleT;//΢�ֲ���Td/T������Ԥ�����ı仯��������ǰ����
	st = 20;//�趨Ŀ���¶�
}
//����PID���uk
void PID_Operation(void)
{
	pt = readThermistor();//��ǰ�¶�ֵ
	e0=st-pt;
	if(e0>10) uk = 100;//�²�>10��ʱ��ȫ�ټ���
	else//�������PID����
	{
		duk=kp*(e0-e1)+kp*ki*e0+kp*kd*(e0-2*e1+e2);
		uk=uk+duk;
		if(uk>100) uk=100;//��ֹ����
		if(uk<0) uk=0;
	}
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
		HEAT=0; //������
	else if(heatFlag) HEAT = 1;//����
	else HEAT=0; //������
	if(uk) uk--;                //ֻ��uk>0�����б�Ҫ����1��
//	if(timecount%10==0) printf("uk:%d,e0:%2.1f,e1:%2.1f,e2:%2.1f\r\n",uk,e0,e1,e2);
	timecount++;
	if(timecount >= 100)
	{
		PID_Operation();        //ÿ��0.1*100s����һ��PID���㡣
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
	
	//STM32û�г����ͷ�PB3��Ϊ��ͨIO��ʹ�ã��л���SW���Կ��ͷ�PB3��PB4��PA15
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO,ENABLE);
	GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable,ENABLE);
	DBGMCU_CR &=0xFFFFFFDF;  //���û����δ��룬PB3�ͻ�һֱ�ǵ͵�ƽ
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

void TIM4_IRQHandler(void)
{
	if(TIM_GetITStatus(TIM4, TIM_IT_Update)!=RESET)//���TIM4�����жϷ������
	{
		TIM_ClearITPendingBit(TIM4, TIM_IT_Update);//���TIM4�����жϱ�־
		PID_Output();//ÿ0.01s����һ��PID
		nowTime++;
		sleepCount++;
		shutCount++;
	}
}

void getClockTime(char timeStr[])
{
	u8 hour=0,min=0,sec=0;
	sec = nowTime/100;
	hour = sec/3600;
	min = sec%3600/60;
	sec = sec-hour*3600-min*60;
	sprintf((char *)timeStr,"%02d:%02d:%02d",hour,min,sec);//���ʱ���ַ���
}

