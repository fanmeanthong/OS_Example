#ifndef __UART_H // 
#define __UART_H

#include "stm32f10x.h"
#include <stdio.h>
#include <stdlib.h>
void UART1_Init(void) ;

void UART1_SendChar(char c);

void UART1_SendString(const char *str);

void log_stack(uint32_t *sp);

#endif