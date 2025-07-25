#ifndef __TIMEBASE_H // 
#define __TIMEBASE_H
#include "stm32f10x.h"
void Sys_Init(void);

void Tick_Increment(void);

uint32_t get_tick(void);

void SysTick_Handler(void);

void delay(uint32_t delay);

#endif
