#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

#include "stm32f10x.h"

size_t stdout_write(FILE *stream, const char *ptr, size_t size)
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

static const struct File_methods stdout_methods = {
    stdout_write,
    NULL
};

static const struct File _stdout = {&stdout_methods};
FILE* const stdout = (FILE*)&_stdout;
FILE* const stderr = (FILE*)&_stdout;


