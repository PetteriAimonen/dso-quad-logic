/* This is an alternative implementation of the LCD routines in the BIOS.
 * 
 * It is needed because the signal capture uses DMA to read data from FPGA.
 * The FPGA and LCD are on the same FSMC memory bus. STM32F103 revision Z
 * has a CPU bug where accessing the FSMC from two places (DMA1, DMA2, CPU)
 * at once will cause a crash.
 * 
 * This code uses the DMA1 controller to serialize the accesses. This means
 * that we do some silly small DMA transactions to write to the LCD.
 */

#include "stm32f10x.h"
#include "BIOS.h"
#include "lcd.h"

#define LCD_RS_LOW()      GPIOD->BRR  = (1<<12)
#define LCD_RS_HIGH()     GPIOD->BSRR = (1<<12)

#define ILI9327_SET_COLUMN_ADDR    0x2A
#define ILI9327_SET_PAGE_ADDR      0x2B
#define ILI9327_WRITE_MEMORY       0x2C
#define ILI9327_READ_MEMORY        0x2E
#define ILI9327_SET_ADDR_MODE      0x36
#define ILI9327_DEVICE_CODE_READ   0xEF

#define R61509V_REG_HORI_ADDR      0x200
#define R61509V_REG_VERT_ADDR      0x201
#define R61509V_CMD_DATA_ACCESS    0x202

uint32_t LCD_TYPE;

void lcd_init()
{
    LCD_TYPE = lcd_get_type();
}

// Wait for LCD DMA transfer to complete
void lcd_dma_ready()
{
    if (DMA1_Channel2->CCR & 1)
    {
        while (DMA1_Channel2->CNDTR != 0);
        DMA1_Channel2->CCR &= ~1; // Disable the channel
    }
}

void lcd_write_dma(const uint16_t *buffer, int count)
{
    lcd_dma_ready();
    DMA1_Channel2->CMAR = (uint32_t)buffer;
    DMA1_Channel2->CNDTR = count;
    DMA1_Channel2->CPAR = 0x60000000;
    DMA1_Channel2->CCR = 0x5591;
}

void lcd_read_dma(uint16_t *buffer, int count)
{
    lcd_dma_ready();
    DMA1_Channel2->CMAR = (uint32_t)buffer;
    DMA1_Channel2->CNDTR = count;
    DMA1_Channel2->CPAR = 0x60000000;
    DMA1_Channel2->CCR = 0x5581;
}

void lcd_write(uint16_t value)
{
    lcd_write_dma(&value, 1);
}

// Write a command word
void lcd_write_cmd(int command)
{
    lcd_dma_ready();
    LCD_RS_LOW();
    lcd_write(command);
    lcd_dma_ready();
    LCD_RS_HIGH();
}

// Write register
void lcd_write_reg(int reg, int command)
{
    lcd_dma_ready();
    LCD_RS_LOW();
    lcd_write(reg);
    lcd_dma_ready();
    LCD_RS_HIGH();
    lcd_write(command);
}

uint32_t lcd_get_type()
{
    uint16_t buffer[5] = {0};
    lcd_write_cmd(ILI9327_DEVICE_CODE_READ);
    lcd_read_dma(buffer, 5);
    lcd_dma_ready();
    
    uint32_t result = 0;
    for (int i = 1; i < 5; i++)
        result = (result << 8) | (buffer[i] & 0xFF);
    return result;
}

void lcd_set_location(int x0, int y0)
{
    if (LCD_TYPE == LCD_TYPE_ILI9327)
    {
        lcd_write_cmd(ILI9327_SET_COLUMN_ADDR);
        uint16_t buffer[4] = {
            0, (uint16_t)(y0 & 0xFF), // Start row
            0, 0xEF // End row = 239
        };
        lcd_write_dma(buffer, 4);
        
        lcd_write_cmd(ILI9327_SET_PAGE_ADDR);
        uint16_t buffer2[4] = {
            (uint16_t)(x0 >> 8), (uint16_t)(x0 & 0xFF), // Start column
            0x01, 0x8F // End column = 399
        };
        lcd_write_dma(buffer2, 4);
        lcd_write_cmd(ILI9327_WRITE_MEMORY);
    }
    else
    {
        lcd_write_reg(R61509V_REG_HORI_ADDR, y0);
        lcd_write_reg(R61509V_REG_VERT_ADDR, x0);
        lcd_write_cmd(R61509V_CMD_DATA_ACCESS);
    }
}

void lcd_getcolumn(int x0, int y0, uint16_t *column, int count)
{
    if (LCD_TYPE == LCD_TYPE_ILI9327)
    {
        uint16_t dummy;
        lcd_set_location(x0, y0);
        lcd_write_cmd(ILI9327_READ_MEMORY);
        lcd_read_dma(&dummy, 1);
        lcd_read_dma(column, count);
    }
    else
    {
        // I haven't been able to test this code path.
        // The R61509V doesn't automatically increment the address when
        // reading, which slows this down a bit...
        lcd_write_reg(R61509V_REG_VERT_ADDR, x0);
        while (count--)
        {
            uint16_t buffer[2];
            lcd_write_reg(R61509V_REG_HORI_ADDR, y0++);
            lcd_write_cmd(R61509V_CMD_DATA_ACCESS);
            lcd_read_dma(buffer, 2);
            lcd_dma_ready();
            *column++ = buffer[1];
        }
    }
}

uint16_t lcd_getpixel(int x0, int y0)
{
    uint16_t result;
    lcd_getcolumn(x0, y0, &result, 1);
    lcd_dma_ready();
    return result;
}
