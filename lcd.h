// Workaround routines for accessing the LCD always through DMA.
#pragma once

#include <stdint.h>

#define LCD_TYPE_ILI9327 0x02049327

extern uint32_t LCD_TYPE;

void lcd_init();
void lcd_dma_ready();
void lcd_write_dma(const uint16_t *buffer, int count);
void lcd_read_dma(uint16_t *buffer, int count);
void lcd_write(uint16_t value);
void lcd_write_cmd(int command);
void lcd_write_reg(int reg, int command);
uint32_t lcd_get_type();
void lcd_set_location(int x0, int y0);
void lcd_getcolumn(int x0, int y0, uint16_t *column, int count);
void lcd_getrow(int x0, int y0, uint16_t *row, int count);
uint16_t lcd_getpixel(int x0, int y0);