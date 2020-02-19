#include "sdcard.h"

GPIO_InitTypeDef GPIO_InitStructure;
SPI_InitTypeDef SPI_InitStructure;

void SetPinsForSPI2(void)
{
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
}

void ConfigureSPI2AsMaster(void)
{
	SetPinsForSPI2();
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);
	SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_16b;
	SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_32; 
	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB; 
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master; 
	SPI_InitStructure.SPI_CRCPolynomial = 7;
	
	SPI_Init(SPI2, &SPI_InitStructure); 
	SPI_Cmd(SPI2, ENABLE);
	SPI_CalculateCRC(SPI2, ENABLE);	
}

void SendSPIData(uint16_t data)
{
	SPI_I2S_SendData(SPI2, data);
}

//void DMAInit_SPI2Sending(uint16_t *data, uint16_t len)
//{
//	DMA_InitTypeDef DMA_InitStructure;
//	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&SPI2->DR;                 
//	DMA_InitStructure.DMA_MemoryBaseAddr =(uint32_t)data;                      
//	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;                            
//	DMA_InitStructure.DMA_BufferSize = len;                                          
//	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;                         
//	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;                 
//	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;              
//	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord ;   
//	DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;                                   
//	DMA_InitStructure.DMA_Priority = DMA_Priority_Medium;                            
//	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;                                    
//	DMA_Init (DMA1_Channel5, &DMA_InitStructure);
//	SPI_I2S_DMACmd(SPI2, SPI_I2S_DMAReq_Tx, ENABLE);
//	DMA_Cmd(DMA1_Channel5, ENABLE);
//	
//	DMA_ITConfig(DMA1_Channel5, DMA_IT_TC, ENABLE);
//	NVIC_EnableIRQ(DMA1_Channel5_IRQn);
//}