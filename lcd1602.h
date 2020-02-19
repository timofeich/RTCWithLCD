#include "stm32f10x_i2c.h"


void I2CInit(void);
void lcd_send_cmd (char cmd);
void lcd_send_data (char data);
void lcd_init (void);
void lcd_send_string (char *str);
void Display_SetXY(uint8_t x, uint8_t y);
void Display_Print(char * string, uint8_t x, uint8_t y);