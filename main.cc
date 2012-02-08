#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

extern "C" {
#include "BIOS.h"
#include "stm32f10x.h"
#include "ds203_io.h"
#include "mathutils.h"
#include "Interrupt.h"
#include "irq.h"
}

#include "dsosignalstream.hh"
#include "xposhandler.hh"
#include "drawable.hh"
#include "textdrawable.hh"
#include "signalgraph.hh"
#include "breaklines.hh"
#include "window.hh"
#include "cursor.hh"

// For some reason, the headers don't have this register
#define FSMC_BTR1   (*((vu32 *)(0xA0000000+0x04)))
#define FSMC_BTR2   (*((vu32 *)(0xA0000008+0x04)))

// GPIOC->BSRR values to toggle GPIOC5
static const uint32_t hl_set[2] = {1 << (16 + 5), 1 << 5};

// FIFO for stuff that is coming from the DMA.
static uint32_t adc_fifo[256];
#define ADC_FIFO_HALFSIZE (sizeof(adc_fifo) / sizeof(uint32_t) / 2)

struct signal_buffer_t signal_buffer = {0, 0};

// This function is the hotspot of the whole capture process.
// It compares the samples until it finds an edge.
const uint32_t * __attribute__((optimize("O3")))
find_edge(const uint32_t *data, const uint32_t *end, const uint32_t mask, const uint32_t old)
{
    // Get to a 4xsizeof(int) boundary
    while (((uint32_t)data & 0x0F) != ((uint32_t)adc_fifo & 0x0F))
    {
        if ((*data & mask) != old) return data;
        data++;
    }
    
    while (data < end)
    {
        if ((*data & mask) != old) return data;
        data++;
        if ((*data & mask) != old) return data;
        data++;
        if ((*data & mask) != old) return data;
        data++;
        if ((*data & mask) != old) return data;
        data++;
    }
    
    return end;
}

static void
process_samples(const uint32_t *data) 
{
    static uint32_t old = 0;
    static signaltime_t count = 0;
    
    // Compare the second-highest bit of each channel and the digital inputs.
    const uint32_t mask = 0x00034040;
    
    const uint32_t *end = data + ADC_FIFO_HALFSIZE;
    for(;;)
    {
        const uint32_t *start = data;
        data = find_edge(data, end, mask, old);
        
        // Update count
        count += data - start;
        
        if (data == end)
            break; // All done.
        
        // Just a sanity-check
        if (*data & 0xFF000000)
        {
            debugf("Too bad, lost the H_L sync");
            while(1);
        }
        
        // We may need up to 10 bytes of space in the buffer
        if (sizeof(signal_buffer.storage) < signal_buffer.bytes + 10)
        {
            // Buffer is full
            NVIC_DisableIRQ(DMA1_Channel4_IRQn);
            return;
        }

        // Write the value as base-128 varint (google protobuf-style)
        uint64_t value_to_write = (count << 4) + signal_buffer.last_value;
        uint8_t *p = signal_buffer.storage + signal_buffer.bytes;
        int i = 0;
        while (value_to_write)
        {
            p[i] = (value_to_write & 0x7F) | 0x80;
            value_to_write >>= 7;
            i++;
        }
        p[i - 1] &= 0x7F; // Unset top bit on last byte
        signal_buffer.bytes += i;

        // Prepare for seeking the next edge
        old = (*data & mask);
        count = 0;
        
        signal_buffer.last_value = 0;
        if (*data & 0x00000040) signal_buffer.last_value |= 1; // Channel A
        if (*data & 0x00004000) signal_buffer.last_value |= 2; // Channel B
        if (*data & 0x00010000) signal_buffer.last_value |= 4; // Channel C
        if (*data & 0x00020000) signal_buffer.last_value |= 8; // Channel D
    }
    
    signal_buffer.last_duration = count;
}

void __irq__ DMA1_Channel4_IRQHandler()
{
    if (DMA1->ISR & DMA_ISR_TEIF4)
    {
        debugf("Oh noes: DMA channel 4 transfer error!");
        while(1);
    }
    else if (DMA1->ISR & DMA_ISR_HTIF4)
    {
        process_samples(&adc_fifo[0]);
        DMA1->IFCR = DMA_IFCR_CHTIF4;
        if (DMA1->ISR & DMA_ISR_TCIF4)
        {
            debugf("Oh noes: ADC fifo overflow in HTIF");
            while(1);
        }
    }
    else if (DMA1->ISR & DMA_ISR_TCIF4)
    {
        process_samples(&adc_fifo[ADC_FIFO_HALFSIZE]);
        DMA1->IFCR = DMA_IFCR_CTCIF4;
        if (DMA1->ISR & DMA_ISR_HTIF4)
        {
            debugf("Oh noes: ADC fifo overflow in TCIF");
            while(1);
        }
    }
}

void draw_screen(const std::vector<Drawable*> &objs, int startx, int endx)
{
    const int screenheight = 240;
    uint16_t buffer1[screenheight];
    uint16_t buffer2[screenheight];
    
    for (Drawable *d: objs)
    {
        d->Prepare(startx, endx);
    }
    
    __Point_SCR(startx, 0);
    for (int x = startx; x < endx; x++)
    {
        uint16_t *buffer = (x % 2) ? buffer1 : buffer2;
        memset(buffer, 0, screenheight * 2);
        
        for (Drawable *d: objs)
        {
            d->Draw(buffer, screenheight, x);
        }
        
        __LCD_DMA_Ready();
        __LCD_Copy(buffer, screenheight);
    }
}

#include "gpio.h"
DECLARE_GPIO(usart1_tx, GPIOA, 9);
DECLARE_GPIO(usart1_rx, GPIOA, 10);

int main(void)
{   
    __Set(BEEP_VOLUME, 0);
    
    // USART1 8N1 115200bps debug port
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
    USART1->BRR = ((72000000 / (16 * 115200)) << 4) | 1;
    USART1->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
    gpio_usart1_tx_mode(GPIO_AFOUT_10);
    gpio_usart1_rx_mode(GPIO_HIGHZ_INPUT);
    
    __Set(ADC_CTRL, EN);       
    __Set(ADC_MODE, SEPARATE);               

    __Set(CH_A_COUPLE, DC);
    __Set(CH_A_RANGE, ADC_1V);
    
    __Set(CH_B_COUPLE, DC);
    __Set(CH_B_RANGE, ADC_1V);
    
    __Set(TRIGG_MODE, UNCONDITION);
    __Set(T_BASE_PSC, 0);
    __Set(T_BASE_ARR, 5);
    __Set(CH_A_OFFSET, 0);
    __Set(CH_B_OFFSET, 0);
    __Set_Param(FPGA_SP_PERCNT_L, 0);
    __Set_Param(FPGA_SP_PERCNT_H, 0);
    
    __Read_FIFO();
    __Read_FIFO();
    
    while (~__Get(KEY_STATUS) & ALL_KEYS);
    DelayMs(500); // Wait for ADC to settle
    
    // Samplerate is 6 MHz, two TMR1 cycles per sample -> ARR = 6 - 1
    // Channel 1: MCO to sample ADC
    // Channel 2: Trigger DMA Ch3 to write H_L bit
    // Channel 4: Trigger DMA Ch4 to read data to memory
    //
    // TMR cycle:    0  1  2  3  4  5  0  1  2  3  4  5 0
    // MCO output:  _|^^^^^^^^^^^^^^^^^|________________|^
    // H_L:         _|^^^^^^^^^^^^^^^^^|________________|^
    // DMA sample:         ^ read ch A&B     ^ read ch C&D
    TIM1->CR1 = 0; // Turn off TIM1 until we are ready
    TIM1->CR2 = 0;
    TIM1->CNT = 0;
    TIM1->SR = 0;
    TIM1->PSC = 11;
    TIM1->ARR = 5;
    TIM1->CCMR1 = 0x0030; // CC2 time base, CC1 toggle output
    TIM1->CCMR2 = 0x0000; // CC4 time base
    TIM1->DIER = TIM_DIER_CC2DE | TIM_DIER_CC4DE;
    TIM1->CCR1 = 0;
    TIM1->CCR2 = 0;
    TIM1->CCR4 = 3;
    
    // DMA1 channel 3: copy data from hl_set to GPIOC->BSRR
    // Priority: very high
    // MSIZE = PSIZE = 32 bits
    // MINC enabled, CIRC mode enabled
    // Direction: read from memory
    // No interrupts
    DMA1_Channel3->CCR = 0;
    DMA1_Channel3->CNDTR = 2;
    DMA1_Channel3->CPAR = (uint32_t)&GPIOC->BSRR;
    DMA1_Channel3->CMAR = (uint32_t)hl_set;
    DMA1_Channel3->CCR = 0x3AB1;
    
    // DMA1 channel 4: copy data from FPGA to adc_fifo.
    // Priority: very high
    // MSIZE = PSIZE = 16 bits
    // MINC enabled, CIRC mode enabled
    // Direction: read from peripheral
    // Half- and Full-transfer interrupts, plus error interrupt
    DMA1_Channel4->CCR = 0;
    DMA1_Channel4->CNDTR = sizeof(adc_fifo) / 2;
    DMA1_Channel4->CPAR = 0x64000000; // FPGA memory-mapped address
    DMA1_Channel4->CMAR = (uint32_t)adc_fifo;
    DMA1_Channel4->CCR = 0x35AF;
    
    // Reduce the wait states of the FPGA & LCD interface
    FSMC_BTR1 = 0x10100110;
    FSMC_BTR2 = 0x10100110;
    
    // Downgrade the LCD transfer priority to medium
    DMA1_Channel1->CCR &= ~DMA_CCR1_PL_1;
    DMA1_Channel2->CCR &= ~DMA_CCR1_PL_1;
    
    // Clear any pending interrupts for ch 4
    DMA1->IFCR = 0x0000F000;
    
    // Enable ch 4 interrupt
    NVIC_EnableIRQ(DMA1_Channel4_IRQn);
    NVIC_SetPriority(DMA1_Channel4_IRQn, 0); // Highest priority
    
    // Now, lets go!
    TIM1->CR1 |= TIM_CR1_CEN;
    
    DSOSignalStream stream(&signal_buffer);
    XPosHandler xpos(400, stream);
    
    std::vector<Drawable*> screenobjs;
    Window graphwindow(64, 0, 400, 240);
    screenobjs.push_back(&graphwindow);

    uint16_t colors[4] = {0xFFE0, 0x07FF, 0xF81F, 0x07E0};
    char names[4][6] = {"CH(A)", "CH(B)", "CH(C)", "CH(D)"};
    for (int i = 0; i < 4; i++)
    {
        SignalGraph* graph = new SignalGraph(stream, &xpos, i);
        graph->y0 = 150 - i * 30;
        graph->color = colors[i];
        if (i == 0 || i == 1)
        {
            // FIXME: A hack for missync in input channels
            graph->offset = 6;
        }
        
        graphwindow.items.push_back(graph);
        
        int middle_y = graph->y0 + graph->height / 2;
        TextDrawable* text = new TextDrawable(50, middle_y, names[i]);
        text->valign = TextDrawable::MIDDLE;
        text->halign = TextDrawable::RIGHT;
        text->color = colors[i];
        screenobjs.push_back(text);
    }
    
    BreakLines breaklines(&xpos, 500000);
    breaklines.linecolor = RGB565RGB(127, 127, 127);
    breaklines.textcolor = RGB565RGB(127, 127, 127);
    breaklines.y0 = 50;
    breaklines.y1 = 180;
    graphwindow.items.push_back(&breaklines);
    
    Cursor cursor(&xpos);
    cursor.linecolor = 0x00FF;
    graphwindow.items.push_back(&cursor);
    
    TextDrawable button1txt(0, 240, " CLEAR ");
    button1txt.invert = true;
    screenobjs.push_back(&button1txt);
    
    TextDrawable button2txt(65, 240, " SAVE ");
    button2txt.invert = true;
    screenobjs.push_back(&button2txt);
    
    TextDrawable button3txt(130, 240, " BMP ");
    button3txt.invert = true;
    screenobjs.push_back(&button3txt);
    
    TextDrawable statustext(390, 0, "");
    statustext.halign = TextDrawable::RIGHT;
    statustext.valign = TextDrawable::BOTTOM;
    screenobjs.push_back(&statustext);
    
    int moves = 0;
    uint32_t old_keys = 0;
    
    while(1) {
        xpos.set_zoom(xpos.get_zoom());
        
        // Report some status
        char buffer[50];
        size_t free_bytes, largest_block;
        get_malloc_memory_status(&free_bytes, &largest_block);
        snprintf(buffer, sizeof(buffer), "Capture buffer: %2ld %%  Free RAM: %4d B",
                 div_round(signal_buffer.bytes * 100, sizeof(signal_buffer.storage)),
                 free_bytes);
        statustext.set_text(buffer);
        
        draw_screen(screenobjs, 0, 400);
        
        uint32_t keys = 0;
        int count = 0;
        while (!(keys & ALL_KEYS) && count++ < 100)
        {
            DelayMs(10);
            keys = ~__Get(KEY_STATUS);
        }
        
        if (keys & KEY1_STATUS)
        {
            signal_buffer.last_duration = 0;
            signal_buffer.bytes = 0;
            xpos.set_xpos(0);
        }
        
        if (keys & KEY2_STATUS)
        {
            clearline(0);
            stream.seek(0);
            
            char *name = select_filename("waves%03d.vcd");
            debugf("Writing data to %s ", name);
            
            _fopen_wr(name);
            _fprintf("$version DSO Quad Logic Analyzer $end\n");
            _fprintf("$timescale 2us $end\n");
            _fprintf("$scope module logic $end\n");
            _fprintf("$var wire 1 A ChannelA $end\n");
            _fprintf("$var wire 1 B ChannelB $end\n");
            _fprintf("$var wire 1 C ChannelC $end\n");
            _fprintf("$var wire 1 D ChannelD $end\n");
            _fprintf("$upscope $end\n");
            _fprintf("$enddefinitions $end\n");
            _fprintf("$dumpvars 0A 0B 0C 0D $end\n");
            
            SignalEvent event;
            while (stream.read_forwards(event))
            {
                _fprintf("#%lu %dA %dB %dC %dD\n",
                         (uint32_t)event.start,
                         !!(event.levels & 1), !!(event.levels & 2),
                         !!(event.levels & 4), !!(event.levels & 8)
                );
            }
            
            _fprintf("#%lu\n", (uint32_t)event.end);
            
            clearline(0);
            if (_fclose())
            {
                debugf("%s successfully written", name);
            }
            else
            {
                debugf("Failed to write file.");
            }
            
            DelayMs(3000);
        }
        
        if (keys & KEY3_STATUS)
        {
            clearline(0);
            char *name = select_filename("logic%03d.bmp");
            debugf("Writing screenshot to %s ", name);
            
            if (write_bitmap(name))
            {
                clearline(0);
                debugf("Wrote %s successfully!", name);
            }
            else
            {
                clearline(0);
                debugf("Bitmap write failed.");
            }
            
            DelayMs(3000);
        }
        
        if (keys & KEY4_STATUS)
        {
            clearline(0);
            debugf("Dumping memory");
            _fopen_wr("memory.dmp");
            for (char *p = (char*)0x20000000; p < (char*)0x2000C000; p++)
                _fputc(*p);
            
            clearline(0);
            if (_fclose())
                debugf("Success!");
            else
                debugf("Fail!");
            DelayMs(500);
        }
        
        if ((keys & (K_ITEM_D_STATUS | K_ITEM_I_STATUS))
            && keys == old_keys)
            moves++;
        else
            moves = 0;
        old_keys = keys;
        
        int move_speed = 1;
        if (moves > 50)
            move_speed = 20;
        else if (moves > 10)
            move_speed = 10;
        else if (moves > 5)
            move_speed = 5;
        else if (moves > 2)
            move_speed = 2;
        
        if (keys & K_ITEM_D_STATUS)
            xpos.move_xpos(-move_speed);

        if (keys & K_ITEM_I_STATUS)
            xpos.move_xpos(move_speed);
        
        int zoom = xpos.get_zoom();
        if ((keys & K_INDEX_D_STATUS) && zoom > -30)
            xpos.set_zoom(zoom - 1);
        
        if ((keys & K_INDEX_I_STATUS) && zoom < 3)
            xpos.set_zoom(zoom + 1);

        printf("XPOS %lu\n", (uint32_t)xpos.get_xpos());
        
        while (~__Get(KEY_STATUS) & (ALL_KEYS ^ K_ITEM_D_STATUS ^ K_ITEM_I_STATUS));
    }
    
    return 0;
}

