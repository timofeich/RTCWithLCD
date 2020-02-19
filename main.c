#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rtc.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_pwr.h"
#include "stdio.h"

#include "tim2_delay.h"
#include "lcd1602.h"


//#include "ADC.h"

void SetSysClockToHSE(void)
{
	ErrorStatus HSEStartUpStatus;
  
	RCC_DeInit();
	RCC_HSEConfig(RCC_HSE_ON);
	HSEStartUpStatus = RCC_WaitForHSEStartUp();

	if (HSEStartUpStatus == SUCCESS)
	{
		RCC_HCLKConfig(RCC_SYSCLK_Div1);
		RCC_PCLK2Config(RCC_HCLK_Div1);
		RCC_PCLK1Config(RCC_HCLK_Div1);
		RCC_SYSCLKConfig(RCC_SYSCLKSource_HSE);
		
		while (RCC_GetSYSCLKSource() != 0x04)
		{ 	
		}
  }
    else
    { 
        while (1)
        {
        }
    }
}

uint8_t RTC_Init(void)
{
	// Включить тактирование модулей управления питанием и управлением резервной областью
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);
	// Разрешить доступ к области резервных данных
	PWR_BackupAccessCmd(ENABLE);
	// Если RTC выключен - инициализировать
	if ((RCC->BDCR & RCC_BDCR_RTCEN) != RCC_BDCR_RTCEN)
	{
		// Сброс данных в резервной области
		RCC_BackupResetCmd(ENABLE);
		RCC_BackupResetCmd(DISABLE);

		// Установить источник тактирования кварц 32768
		RCC_LSEConfig(RCC_LSE_ON);
		while ((RCC->BDCR & RCC_BDCR_LSERDY) != RCC_BDCR_LSERDY) {}
		BKP->RTCCR |= 3;        
		RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);
			
		RTC_SetPrescaler(0x7FFF); // Устанавливаем делитель, чтобы часы считали секунды
		// Включаем RTC
		RCC_RTCCLKCmd(ENABLE);

		// Ждем синхронизацию
		RTC_WaitForSynchro();
			return 1;
	}
	
	return 0;
}

// (UnixTime = 00:00:00 01.01.1970 = JD0 = 2440588)
#define JULIAN_DATE_BASE	2440588

typedef struct
{
	uint8_t RTC_Hours;
	uint8_t RTC_Minutes;
	uint8_t RTC_Seconds;
	uint8_t RTC_Date;
	uint8_t RTC_Wday;
	uint8_t RTC_Month;
	uint16_t RTC_Year;
} RTC_DateTimeTypeDef;

// Get current date
void RTC_GetDateTime(uint32_t RTC_Counter, RTC_DateTimeTypeDef* RTC_DateTimeStruct) 
{
	unsigned long time;
	unsigned long t1, a, b, c, d, e, m;
	int year = 0;
	int mon = 0;
	int wday = 0;
	int mday = 0;
	int hour = 0;
	int min = 0;
	int sec = 0;
	uint64_t jd = 0;;
	uint64_t jdn = 0;

	jd = ((RTC_Counter + 43200) / (86400 >> 1)) + (2440587 << 1) + 1;
	jdn = jd >> 1;

	time = RTC_Counter;
	t1 = time / 60;
	sec = time - t1 * 60;

	time = t1;
	t1 = time / 60;
	min = time - t1 * 60;

	time = t1;
	t1 = time / 24;
	hour = time - t1 * 24;

	wday = jdn % 7;

	a = jdn + 32044;
	b = (4 * a + 3) / 146097;
	c = a - (146097 * b) / 4;
	d = (4 * c + 3) / 1461;
	e = c - (1461 * d) / 4;
	m = (5 * e + 2) / 153;
	mday = e - (153 * m + 2) / 5 + 1;
	mon = m + 3 - 12 * (m / 10);
	year = 100 * b + d - 4800 + (m / 10);

	RTC_DateTimeStruct -> RTC_Year = year;
	RTC_DateTimeStruct -> RTC_Month = mon;
	RTC_DateTimeStruct -> RTC_Date = mday;
	RTC_DateTimeStruct -> RTC_Hours = hour;
	RTC_DateTimeStruct -> RTC_Minutes = min;
	RTC_DateTimeStruct -> RTC_Seconds = sec;
	RTC_DateTimeStruct -> RTC_Wday = wday;
}

// Convert Date to Counter
uint32_t RTC_GetRTC_Counter(RTC_DateTimeTypeDef* RTC_DateTimeStruct) 
{
	uint8_t a;
	uint16_t y;
	uint8_t m;
	uint32_t JDN;

	a = (14 - RTC_DateTimeStruct -> RTC_Month) / 12;
	y = RTC_DateTimeStruct -> RTC_Year + 4800 - a;
	m = RTC_DateTimeStruct -> RTC_Month + (12 * a) - 3;

	JDN = RTC_DateTimeStruct -> RTC_Date;
	JDN += (153 * m + 2) / 5;
	JDN += 365 * y;
	JDN += y / 4;
	JDN += -y / 100;
	JDN += y / 400;
	JDN = JDN - 32045;
	JDN = JDN - JULIAN_DATE_BASE;
	JDN *= 86400;
	JDN += (RTC_DateTimeStruct -> RTC_Hours * 3600);
	JDN += (RTC_DateTimeStruct -> RTC_Minutes * 60);
	JDN += (RTC_DateTimeStruct -> RTC_Seconds);

	return JDN;
}

uint16_t ADCBuffer[] = {0x0000, 0x0000, 0x0000, 0x0000};
void ADC1_Configure(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	
	ADC_InitTypeDef ADC_InitStructure;
    ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
    ADC_InitStructure.ADC_ScanConvMode = DISABLE;
    ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;  // we work in continuous sampling mode
    ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_NbrOfChannel = 1;
 
    ADC_RegularChannelConfig(ADC1, ADC_Channel_1, 1, ADC_SampleTime_28Cycles5); // define regular conversion config
    ADC_Init ( ADC1, &ADC_InitStructure);   //set config of ADC1
 
    ADC_Cmd (ADC1,ENABLE);  //enable ADC1
 
    ADC_ResetCalibration(ADC1); // Reset previous calibration
    while(ADC_GetResetCalibrationStatus(ADC1));
    ADC_StartCalibration(ADC1); // Start new calibration (ADC must be off at that time)
    while(ADC_GetCalibrationStatus(ADC1));

	ADC_Cmd(ADC1 , ENABLE ) ;
    ADC_DMACmd(ADC1 , ENABLE );

    ADC_SoftwareStartConvCmd(ADC1, ENABLE);
}

void DMAInit_ADCRecieve(void)
{
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);	
	DMA_InitTypeDef DMA_InitStructure;
    DMA_InitStructure.DMA_BufferSize = 4;
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
    DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)ADCBuffer;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&ADC1->DR;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_Priority = DMA_Priority_High;
    DMA_Init(DMA1_Channel1, &DMA_InitStructure);
    DMA_Cmd(DMA1_Channel1 , ENABLE ) ;
	
	ADC1_Configure();
}

int main(void)
{	
	char buffer[20] = {'\0'};
	char timeBuffer[20] = {'\0'};
	
	char firstValueADC[17];
	char secondValueADC[17];
	
	uint32_t RTC_Counter = 0;
	RTC_DateTimeTypeDef	RTC_DateTime;

	SetSysClockToHSE();
	TIM2_init();
		
	DMAInit_ADCRecieve();
	I2CInit();
	
	lcd_init();
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC , ENABLE);
	
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin = (GPIO_Pin_9 | GPIO_Pin_8);
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOC , &GPIO_InitStructure);	

	if(RTC_Init() == 1)
	{
		// Если первая инициализация RTC устанавливаем начальную дату
		RTC_DateTime.RTC_Date = 30;
		RTC_DateTime.RTC_Month = 1;
		RTC_DateTime.RTC_Year = 2020;

		RTC_DateTime.RTC_Hours = 16;
		RTC_DateTime.RTC_Minutes = 42;
		RTC_DateTime.RTC_Seconds = 30;

		//После инициализации требуется задержка. Без нее время не устанавливается.
		delay_ms(500);
		RTC_SetCounter(RTC_GetRTC_Counter(&RTC_DateTime));
	}
		
	while(1)
	{
//		if (!RCC_GetFlagStatus(RCC_FLAG_PORRST))

		RTC_Counter = RTC_GetCounter();
		
		RTC_GetDateTime(RTC_Counter, &RTC_DateTime);
		
//		sprintf(buffer, "%02d.%02d.%04d", RTC_DateTime.RTC_Date, RTC_DateTime.RTC_Month, RTC_DateTime.RTC_Year);
//		sprintf(timeBuffer, "%02d:%02d:%02d", RTC_DateTime.RTC_Hours, RTC_DateTime.RTC_Minutes, RTC_DateTime.RTC_Seconds);
//		
//		Display_Print(buffer, 3, 0);
//		Display_Print(timeBuffer, 4, 1);	
		
		GPIOC->ODR ^= (GPIO_Pin_9 | GPIO_Pin_8);
			
		sprintf(firstValueADC, "Ia=%04d  Ib=%04d", ADCBuffer[0], ADCBuffer[1]);
		sprintf(secondValueADC, "Ic=%04d  Id=%04d", ADCBuffer[2], ADCBuffer[3]);
			
		Display_Print(firstValueADC, 0, 0);
		Display_Print(secondValueADC, 0, 1);
		
		while (RTC_Counter == RTC_GetCounter()) 
		{

		}
  }
}


