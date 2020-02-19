#include "stm32f10x.h"
#include "stm32f10x_rcc.h"
#include "lcd1602.h"

uint8_t LCD_ADDR = 0x3F;

void I2CInit(void) 
{
	I2C_InitTypeDef  I2C_InitStructure;
    GPIO_InitTypeDef  GPIO_InitStructure;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);	
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    /* Configure I2C_EE pins: SCL and SDA */
    GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    /* I2C configuration */
    I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
    I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
    I2C_InitStructure.I2C_OwnAddress1 = 0x15;
    I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
    I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
	I2C_InitStructure.I2C_ClockSpeed = 100000;
    /* I2C Peripheral Enable */

	I2C_Cmd(I2C1, ENABLE);
	I2C_Init(I2C1, &I2C_InitStructure);
	
//	NVIC_InitTypeDef  NVIC_InitStructure;
//	NVIC_InitStructure.NVIC_IRQChannel = I2C1_ER_IRQn;
//	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
//	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
//	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
//	NVIC_Init(&NVIC_InitStructure);

	I2C_ITConfig(I2C1, I2C_IT_ERR, ENABLE); //Включаем прерывание по приему байта	
	NVIC_EnableIRQ(I2C1_ER_IRQn);
}

void lcd_init (void)
{
	delay_ms(50);  // wait for >40ms
	lcd_send_cmd (0x30);
	delay_ms(5);  // wait for >4.1ms
	lcd_send_cmd (0x30);
	delay_ms(1);  // wait for >100us
	lcd_send_cmd (0x30);
	delay_ms(10);
	lcd_send_cmd (0x20);  // 4bit mode
	delay_ms(10);

  // dislay initialisation
	lcd_send_cmd (0x28); // Function set --> DL=0 (4 bit mode), N = 1 (2 line display) F = 0 (5x8 characters)
	delay_ms(1);
	lcd_send_cmd (0x08); //Display on/off control --> D=0,C=0, B=0  ---> display off
	delay_ms(1);
	lcd_send_cmd (0x01);  // clear display
	delay_ms(1);
	delay_ms(1);
	lcd_send_cmd (0x06); //Entry mode set --> I/D = 1 (increment cursor) & S = 0 (no shift)
	delay_ms(1);
	lcd_send_cmd (0x0C); //Display on/off control --> D = 1, C and B = 0. (Cursor and blink, last two bits)
}

void lcd_send_cmd (char cmd)
{
	char data_u, data_l;
	uint8_t data_t[4];
	
	data_u = (cmd & 0xf0);
	data_l = ((cmd << 4) & 0xf0);
	
	data_t[0] = data_u | 0x0C;  //en=1, rs=0
	data_t[1] = data_u | 0x08;  //en=0, rs=0
	data_t[2] = data_l | 0x0C;  //en=1, rs=0
	data_t[3] = data_l | 0x08;  //en=0, rs=0
	

	I2C_GenerateSTART(I2C1, ENABLE);
	while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT));
	
	I2C_Send7bitAddress(I2C1, LCD_ADDR << 1, I2C_Direction_Transmitter);
	while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));

	for(int i = 0; i < 4; i++)
	{
		I2C_SendData(I2C1, data_t[i]);
		while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED));
	}
	
	I2C_GenerateSTOP(I2C1, ENABLE);
	while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED));
}

void lcd_send_data (char data)
{
	char data_u, data_l;
	uint8_t data_t[4];
	
	data_u = (data&0xf0);
	data_l = ((data<<4)&0xf0);
	
	data_t[0] = data_u|0x0D;  //en=1, rs=1
	data_t[1] = data_u|0x09;  //en=0, rs=1
	data_t[2] = data_l|0x0D;  //en=1, rs=1
	data_t[3] = data_l|0x09;  //en=0, rs=1
	
	I2C_GenerateSTART(I2C1,ENABLE);
	while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT));
	
	I2C_Send7bitAddress(I2C1, LCD_ADDR << 1, I2C_Direction_Transmitter);
	while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));

	for(int i = 0; i < 4; i++)
	{
		I2C_SendData(I2C1, data_t[i]);
		while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED));
	}
	I2C_GenerateSTOP(I2C1, ENABLE);
	while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED));

}

void Display_SetXY(uint8_t x, uint8_t y) {
	uint8_t addr = x + (y * 0x40);
	lcd_send_cmd(0x80 | addr);
}

void lcd_send_string (char *str)
{
	while (*str) lcd_send_data (*str++);
}

void Display_Print(char * string, uint8_t x, uint8_t y) {
	Display_SetXY(x,y);
	lcd_send_string(string);
}

void I2C1_ER_IRQHandler(void)
{
	I2C_ClearITPendingBit(I2C1, I2C_IT_ERR);
	
//	switch(I2C_GetLastEvent(I2C1))
//    {
//		case I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED :
//			break;
//		case I2C_EVENT_MASTER_BYTE_TRANSMITTED:
//			break;
//		case I2C_EVENT_MASTER_BYTE_TRANSMITTING:
//			break;
//		case I2C_EVENT_MASTER_MODE_SELECT:
//			break;
//    default:
		//I2C_Send7bitAddress(I2C1, LCD_ADDR << 1, I2C_Direction_Transmitter);	
		lcd_init();
//        break;
//    }
}
