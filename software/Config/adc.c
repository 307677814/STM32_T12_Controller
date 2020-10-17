#include "adc.h"
#include "delay.h"
#include "usart.h"
#include "sys.h"
#include "math.h"
/* 
   ===����������ʾ����===
   Ϊ��������������������ADC���Σ�Ȼ��ƽ�������Ի�ø��ȶ��Ĳ���ֵ����readThermistor����ʵ�֡�
   http://www.thermistors.cn/news/293.html
*/ 


float ADC_max = 4095.0; //������ֵ��12λADC
/*ʹ��beta���̼�����ֵ��*/ 
float beta = 3950.0; //�̼Ҹ����ĵ����Ӧ25��C�µ�bataֵ
float roomTemp = 298.15; //�Կ�����Ϊ��λ������25��C
float balanceR = 9970.0;//�ο�����
float roomTempR = 10000.0; //NTC��������������25��C�¾��е��͵ĵ���
float currentTemperature = 0; //���浱ǰ�¶�
u16 ch1Value[10];//ADC����ֵ
u16 adcAverage=0;
#define ADC1_DR_Address    ((u32)0x4001244C)		//ADC1�ĵ�ַ
//ͨ�ö�ʱ��2�жϳ�ʼ��
//����ʱ��ѡ��ΪAPB1��2������APB1Ϊ36M
//arr���Զ���װֵ��
//psc��ʱ��Ԥ��Ƶ��
//����ʹ�õ��Ƕ�ʱ��2����ADC1��ʱ����
void TIM2_Init(u16 arr,u16 psc)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	TIM_OCInitTypeDef TIM_OCInitStructure;
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE); 		//ʱ��ʹ��

	//��ʱ��TIM2��ʼ��
	TIM_TimeBaseStructure.TIM_Period = arr; 		//��������һ�������¼�װ�����Զ���װ�ؼĴ������ڵ�ֵ
	TIM_TimeBaseStructure.TIM_Prescaler =psc; 			//����������ΪTIMxʱ��Ƶ�ʳ�����Ԥ��Ƶֵ
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1; 		//����ʱ�ӷָ�:TDTS = Tck_tim
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up; 		//TIM���ϼ���ģʽ
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);			//����ָ���Ĳ�����ʼ��TIMx��ʱ�������λ
	
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;		//ѡ��ʱ��ģʽ:TIM�����ȵ���ģʽ1
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;		//�Ƚ����ʹ��
	TIM_OCInitStructure.TIM_Pulse = 9;							//�����ﵽ9�����ж�
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_Low;		//�������:TIM����Ƚϼ��Ե�
	TIM_OC2Init(TIM2, & TIM_OCInitStructure);		//��ʼ������TIM2_CH2
	
	TIM_Cmd(TIM2, ENABLE); 			//ʹ��TIMx
	TIM_CtrlPWMOutputs(TIM2, ENABLE); 
}


//DMA1����
void DMA1_Init(void)
{
	DMA_InitTypeDef DMA_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1,ENABLE);	  			//ʹ��ADC1ͨ��ʱ��
	
	//DMA1��ʼ��
	DMA_DeInit(DMA1_Channel1);
	DMA_InitStructure.DMA_PeripheralBaseAddr = ADC1_DR_Address;				//ADC1��ַ
	DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)&ch1Value; 		//ch1Value���ڴ��ַ
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC; 				//����(�����赽�ڴ�)
	DMA_InitStructure.DMA_BufferSize = 10; 						//DMA�����С�����10�β���ֵ
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable; 	//�����ַ�̶�������һ�����ݺ��豸��ַ��ֹ����
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable; 			//�ڴ��ַ���̶������ն�����ݺ�Ŀ���ڴ��ַ����
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord ; //�������ݵ�λ�������������ݿ��Ϊ16λ
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord ;    //�ڴ����ݵ�λ��HalfWord����Ϊ16λ
	DMA_InitStructure.DMA_Mode = DMA_Mode_Circular  ; 		//DMAģʽ��ѭ������
	DMA_InitStructure.DMA_Priority = DMA_Priority_High ; 	//DMA���ȼ�����
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;   		//��ֹ�ڴ浽�ڴ�Ĵ���
	DMA_Init(DMA1_Channel1, &DMA_InitStructure);  //����DMA1
	
	DMA_ITConfig(DMA1_Channel1,DMA_IT_TC, ENABLE);		//ʹ�ܴ�������ж�

	NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	
	DMA_Cmd(DMA1_Channel1,ENABLE);
}

//�жϴ�����
void  DMA1_Channel1_IRQHandler(void)
{
	if(DMA_GetITStatus(DMA1_IT_TC1)!=RESET){
		//�жϴ������
		adcAverage = GetMedianNum(ch1Value,10);//��ֵ�˲�
		DMA_ClearITPendingBit(DMA1_IT_TC1);//�����־
	}
}

//��ʼ��ADC-PA0����
															   
void  Adc_Init(void)
{ 	
	GPIO_InitTypeDef GPIO_InitStructure;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);	  //ʹ��GPIOAʱ��

	//PA6 ��Ϊģ��ͨ����������   
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_Init(GPIOA, &GPIO_InitStructure);  
	
	ADC_InitTypeDef ADC_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);	  //ʹ��ADC1ͨ��ʱ��

	//ADC1��ʼ��
	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent; 			//����ADCģʽ
	ADC_InitStructure.ADC_ScanConvMode = DISABLE;  				//�ر�ɨ�跽ʽ
	ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;			//�ر�����ת��ģʽ
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T2_CC2;   	//ʹ���ⲿ����ģʽ
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right; 			//�ɼ������Ҷ���
	ADC_InitStructure.ADC_NbrOfChannel = 1; 			//Ҫת����ͨ����Ŀ
	ADC_Init(ADC1, &ADC_InitStructure);
	
	RCC_ADCCLKConfig(RCC_PCLK2_Div6);				//����ADCʱ�ӣ�ΪPCLK2��6��Ƶ����12MHz
	ADC_RegularChannelConfig(ADC1, ADC_Channel_6, 1, ADC_SampleTime_239Cycles5);		//����ADC1ͨ��6Ϊ239.5���������� 
	
	//ʹ��ADC��DMA
	ADC_DMACmd(ADC1,ENABLE);
	ADC_Cmd(ADC1,ENABLE);
 
	ADC_ResetCalibration(ADC1);				//��λУ׼�Ĵ���
	while(ADC_GetResetCalibrationStatus(ADC1));				//�ȴ�У׼�Ĵ�����λ���
 
	ADC_StartCalibration(ADC1);				//ADCУ׼
	while(ADC_GetCalibrationStatus(ADC1));				//�ȴ�У׼���
	
	ADC_ExternalTrigConvCmd(ADC1, ENABLE);		//�����ⲿ����ģʽʹ��
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

/*����˵���������������ֵ�˲���������ֵ
bArray - ���˲������飻iFilterLen - ����Ԫ�ظ���
*/
int GetMedianNum(volatile u16 * bArray, int iFilterLen)
{
    int i,j;// ѭ������
    int bTemp;

    // ��ð�ݷ��������������
    for (j = 0; j < iFilterLen - 1; j ++)
    {
        for (i = 0; i < iFilterLen - j - 1; i ++)
        {
            if (bArray[i] > bArray[i + 1])
            {
                // ����
                bTemp = bArray[i];
                bArray[i] = bArray[i + 1];
                bArray[i + 1] = bTemp;
            }
        }
    }

    // ������ֵ
    if ((iFilterLen & 1) > 0)
    {
        // ������������Ԫ�أ������м�һ��Ԫ��
        bTemp = bArray[(iFilterLen + 1) / 2];
    }
    else
    {
        // ������ż����Ԫ�أ������м�����Ԫ��ƽ��ֵ
        bTemp = (bArray[iFilterLen / 2] + bArray[iFilterLen / 2 + 1]) / 2;
    }

    return bTemp;
}


/* 
  �������ܣ���ȡģ�����ţ�������ʾ��
  ͨ��ģ��ת������ѹ�ź�ת��Ϊ���ֱ�ʾ�����ǣ��������˶�Σ�������ǿ��Զ������ƽ��������������
  Ȼ��ʹ�ø�ƽ������������������ĵ��衣�˺󣬵������ڼ�������������¶ȡ�����¶�ת��Ϊ���϶ȡ�
  �йش˹��̵���ϸ��Ϣ��һ�����ۣ������allaboutcircuits.com���¡�
  ԭ��ͼ��
         3.3V ----====-------  | ---------====--------(GND)
                R_balance      |      R_thermistor 
                               | 
                             ADC����
*/ 
 
float readThermistor(void)
{
  float rThermistor = 0; //������������ĵ���ֵ
  float tKelvin = 0; //�Կ������¶ȱ����¶�
  float tCelsius = 0; //�������¶ȱ����¶�
  /*��ʽ������������ĵ��衣*/ 
  rThermistor = balanceR *adcAverage/(ADC_max - adcAverage); 
  tKelvin =(beta * roomTemp)/(beta +(roomTemp * log(rThermistor / roomTempR)));  
  tCelsius = tKelvin  -  273.15; //��������ת��Ϊ�����¶�
  return tCelsius;//�����϶ȷ����¶�
}
