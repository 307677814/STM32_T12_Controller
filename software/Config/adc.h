#ifndef __ADC_H
#define __ADC_H
#include "stm32f10x.h" //�ǵ���Ӵ�ͷ�ļ�����Ϊconfig.c�õ�GPIO��غ�����
#include "sys.h"

void TIM2_Init(u16 arr,u16 psc);//TIM2��ʱ����ʼ��
void DMA1_Init(void);
void Adc_Init(void);//ADC1��ʼ��
u16 Get_Adc(u8 ch); //��ȡһ��ADC��ֵ
u16 Get_Adc_Average(u8 ch,u8 times);//ADC����ֵ���о�ֵ�˲�
float map(float value,float fromLow,float fromHigh,float toLow,float toHigh);//ӳ�亯��
int GetMedianNum(volatile u16 * bArray, int iFilterLen);//��ֵ�˲�
float readThermistor(void);
#endif
