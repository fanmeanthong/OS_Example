#ifndef __UART_H
#define __UART_H

#include "stm32f10x.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void UART1_Init(void);
void UART1_SendChar(char c);
void UART1_SendString(const char *str);
void log_stack(uint32_t *sp);

/* Retarget printf */
int _write(int file, char *ptr, int len);

void print_str(const char *s);
void print_dec(int value);
void print_hex(uint32_t value);
#ifdef __cplusplus
}
#endif

#endif
