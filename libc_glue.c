#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

#include "stm32f10x.h"

size_t _fread(void *ptr, size_t size, FILE *stream)
{
    return 0;
}

size_t _fwrite(const void *ptr, size_t size, FILE *stream)
{
    // Everything to USART1
    size_t i;
    char *p = (char*)ptr;
    for (i = 0; i < size; i++)
    {
        while (!(USART1->SR & USART_SR_TXE));
        USART1->DR = *p++;
    }
    
    return i;
}