#include "adc.h"
#include "delay.h"
#include "usart.h"
#include "sys.h"
#include "math.h"
#include "main.h"
#include "pid.h"
/* 
   ===����������ʾ����===
   Ϊ��������������������ADC���Σ�Ȼ��ƽ�������Ի�ø��ȶ��Ĳ���ֵ����readThermistor����ʵ�֡�
   http://www.thermistors.cn/news/293.html
*/ 
#define sampleNum 10

float ADC_max = 4095.0; //������ֵ��12λADC
/*ʹ��beta���̼�����ֵ��*/ 
float beta = 3950.0; //�̼Ҹ����ĵ����Ӧ25��C�µ�bataֵ
float roomTemp = 298.15; //�Կ�����Ϊ��λ������25��C
float balanceR = 9900.0;//�ο�����
float roomTempR = 10000.0; //NTC��������������25��C�¾��е��͵ĵ���
float currentTemperature = 0; //���浱ǰ�¶�
u16 ch1Value[2*sampleNum];//ADC����ֵ
u16 NTC_Average=0;
u16 T12_Average=0;
u16 S_temp2Volt[]={
	0,55,113,173,235,299,365,432,502,573,//0~90��
	645,719,795,872,950,1029,1109,1190,1273,1356,//100~190��
	1440,1525,1611,1698,1785,1873,1962,2051,2141,2232,
	2323,2414,2506,2599,2692,2786,2880,2974,3069,3164,
	3260,3356,3452,3549,3645,3743,3840,3938,4036,4135,
	4234,4333,4432,4532,4632,4732,4832,4933,5034,5136,//500~590��
	5237,5339,5442,5544,5648,5751,5855,5960,6065,6169};//S���ȵ�ż�ֶȱ���λ��uV���ο��¶ȣ�0��
u16 S_caliVolt[]={0,55,113,173,235,299};//S���ȵ�ż�ο����¶�0~50��ʱ��У��ֵ��ʵ�ʵ�ѹ-У��ֵ���ٲ�ֶȱ��
#define ADC1_DR_Address    ((u32)0x4001244C)		//ADC1�ĵ�ַ

//��ʼ��ADC-PA0����
															   
void  Adc_Init(void)
{ 	
	GPIO_InitTypeDef GPIO_InitStructure;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);	  //ʹ��GPIOAʱ��

	//PA6 ��Ϊģ��ͨ����������   
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4|GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_Init(GPIOA, &GPIO_InitStructure);  
	
	
	GPIO_InitStructure.GPIO_Pin = SLEEP_Pin;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(SLEEP_GPIO_Port, &GPIO_InitStructure);  
	
	ADC_InitTypeDef ADC_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);	  //ʹ��ADC1ͨ��ʱ��
	RCC_ADCCLKConfig(RCC_PCLK2_Div6);				//����ADCʱ�ӣ�ΪPCLK2��6��Ƶ����12MHz
	
	ADC_DeInit(ADC1);  //��λADC1 
	
	//ADC1��ʼ��
	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent; 			//����ADCģʽ
	ADC_InitStructure.ADC_ScanConvMode = DISABLE;	//ģ��ת�������ڵ�ͨ��ģʽ
	ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;	//ģ��ת�������ڵ���ת��ģʽ
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;	//ת��������������ⲿ��������
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;	//ADC�����Ҷ���
	ADC_InitStructure.ADC_NbrOfChannel = 1;	//˳����й���ת����ADCͨ������Ŀ
	ADC_Init(ADC1, &ADC_InitStructure);	//����ADC_InitStruct��ָ���Ĳ�����ʼ������ADCx�ļĴ���  
	
	
	ADC_RegularChannelConfig(ADC1, ADC_Channel_4, 1, ADC_SampleTime_239Cycles5);		//����ADC1ͨ��4Ϊ239.5����������
	ADC_RegularChannelConfig(ADC1, ADC_Channel_6, 1, ADC_SampleTime_239Cycles5);		//����ADC1ͨ��6Ϊ239.5���������� 
	
	ADC_Cmd(ADC1, ENABLE);	//ʹ��ָ����ADC1
 
	ADC_ResetCalibration(ADC1);				//��λУ׼�Ĵ���
	while(ADC_GetResetCalibrationStatus(ADC1));				//�ȴ�У׼�Ĵ�����λ���
 
	ADC_StartCalibration(ADC1);				//ADCУ׼
	while(ADC_GetCalibrationStatus(ADC1));				//�ȴ�У׼���
//	ADC_SoftwareStartConvCmd(ADC1, ENABLE);		//ʹ��ָ����ADC1�����ת����������
}

//���ADCֵ
//ch:ͨ��ֵ 0~9
u16 Get_Adc(u8 ch)   
{
  	//����ָ��ADC�Ĺ�����ͨ����һ�����У�����ʱ��
	ADC_RegularChannelConfig(ADC1, ch, 1, ADC_SampleTime_239Cycles5 );	//ADC1,ADCͨ��,����ʱ��Ϊ239.5������	
	ADC_SoftwareStartConvCmd(ADC1, ENABLE);		//ʹ��ָ����ADC1�����ת����������	
	while(!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC ));//�ȴ�ת������
	return ADC_GetConversionValue(ADC1);	//�������һ��ADC1�������ת�����
}

//ch:ͨ��ֵ 0~9������times�κ�����ֵ�˲�
u16 Get_Adc_Average(u8 ch,u8 times)
{
	u32 temp_val=0;
	u8 t;
	for(t=0;t<times;t++)
	{
		temp_val+=Get_Adc(ch);
		delay_ms(5);
	}
	return temp_val/times;
}

/*����˵������Arduino,��һ�����ִ�һ����Χ����ӳ�䵽��һ����Χ
Ҳ����˵��fromLow��ֵ��ӳ�䵽toLow��fromlhigh��toHigh��ֵ�ȵȡ�
*/
float map(float value,float fromLow,float fromHigh,float toLow,float toHigh)
{
	return ((value-fromLow)*(toHigh-toLow)/(fromHigh-fromLow)+toLow);
}
/*���ַ����������������е�������ȷ�����ֶ�Ӧ������
ipArray���������ֵ�����
start: ���ҵ���ʼ����0~len
end: ���ҵĽ�������start~len
value: Ҫ���ҵ���ֵ
*/
int SearchIndex(u16 ipArray[], int start, int end ,int  value)  //�����ַ���
{
	int middle = (start + end) / 2;
	if (middle == start)
		return middle;
	else if (value < ipArray[middle])  
		return SearchIndex(ipArray, start, middle, value);
	else
		return SearchIndex(ipArray, middle, end, value);
}
/*�������ܣ���ȡģ�����ţ�������ʾ��
  ͨ��ģ��ת������ѹ�ź�ת��Ϊ���ֱ�ʾ�����ǣ��������˶�Σ�������ǿ��Զ������ƽ��������������
  Ȼ��ʹ�ø�ƽ������������������ĵ��衣�˺󣬵������ڼ�������������¶ȡ�����¶�ת��Ϊ���϶ȡ�
  �йش˹��̵���ϸ��Ϣ��һ�����ۣ������allaboutcircuits.com���¡�
  ԭ��ͼ��
         3.3V ----====-------  | ---------====--------(GND)
                R_balance      |      R_thermistor 
                               | 
                             ADC����
*/ 
u16 get_NTC_temp(void)
{
  float rThermistor = 0; //������������ĵ���ֵ
  float tKelvin = 0; //�Կ������¶ȱ����¶�
  float tCelsius = 0; //�������¶ȱ����¶�
	NTC_Average = Get_Adc_Average(6,10);
  /*��ʽ������������ĵ��衣*/ 
  rThermistor = balanceR * NTC_Average/(ADC_max - NTC_Average); 
  tKelvin =(beta * roomTemp)/(beta +(roomTemp * log(rThermistor / roomTempR)));  
  tCelsius = tKelvin  -  273.15; //��������ת��Ϊ�����¶�
  return tCelsius;//�����϶ȷ����¶�
}
//��ȡ�ȵ�ż�ĵ�ѹ�����ݷֶȱ�ת��Ϊ�¶�
u16 get_T12_temp(void)
{
	u16 nowTemp,nowIndex;
	u16 nowVolt,nowCaliVolt;
	if(HEAT) 
	{
		HEAT=0;//��ֹͣ����
		delay_ms(1);
		T12_Average = Get_Adc_Average(4,10);//��ȡ����ֵ
		HEAT=1;//��������
	}
	else T12_Average = Get_Adc_Average(4,10);//��ȡ����ֵ
	nowCaliVolt = S_caliVolt[(u16)NTC_temp/10]+(NTC_temp%10)*(S_caliVolt[(u16)NTC_temp/10+1]-S_caliVolt[(u16)NTC_temp/10])/10;
	nowVolt = T12_Average*3.3*2000/4095-nowCaliVolt;//�ȵ�ż��ǰ��ѹuV
	nowIndex = SearchIndex(S_temp2Volt,0,70,nowVolt);
	nowTemp = nowIndex*10+10*(nowVolt-S_temp2Volt[nowIndex])/(S_temp2Volt[nowIndex+1]-S_temp2Volt[nowIndex]);
//	printf("%d��\r\n",nowTemp);
	return nowTemp;//�����϶ȷ����¶�
}

u16 get_sleepSign(void)
{
	u16 nowSleep;
	nowSleep = SLEEP;
	if(nowSleep==0)
	{
		sleepCount=0;
		shutCount = 0;
	}
//	printf("%d\r\n",nowSleep);
	return nowSleep;
}
