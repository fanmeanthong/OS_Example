#include "timebase.h"
#define SYSTICK_BASE        (0xE000E010UL)
#define SYSTICK_CTRL        (*(volatile uint32_t *)(SYSTICK_BASE + 0x00))
#define SYSTICK_LOAD        (*(volatile uint32_t *)(SYSTICK_BASE + 0x04))
#define SYSTICK_VAL         (*(volatile uint32_t *)(SYSTICK_BASE + 0x08))

#define SYSTICK_CTRL_ENABLE     (1 << 0)
#define SYSTICK_CTRL_TICKINT    (1 << 1)
#define SYSTICK_CTRL_CLKSOURCE  (1 << 2)
#define MAX_DELAY 0xFFFFFFFF
volatile uint32_t g_curr_tick;
volatile uint32_t g_curr_tick_p;
volatile uint32_t tick_freq =1;

static inline void __enable_irq(void)
{
    __asm volatile ("cpsie i");
}
static inline void __disable_irq(void)
{
    __asm volatile ("cpsid i");
}

void Sys_Init(void) {
    SYSTICK_LOAD = 72000 - 1;                 // Reload after 1ms at 72MHz
    SYSTICK_VAL = 0;                          // Clear current value
    SYSTICK_CTRL = SYSTICK_CTRL_CLKSOURCE     // Use processor clock
                 | SYSTICK_CTRL_TICKINT       // Enable interrupt
                 | SYSTICK_CTRL_ENABLE;       // Enable SysTick
    __enable_irq();                           // Enable global interrupts
}

void Tick_Increment(void){
	g_curr_tick += tick_freq;
    return ;
}

uint32_t get_tick(void){
	__disable_irq();
	g_curr_tick_p = g_curr_tick;
	__enable_irq();
	return g_curr_tick_p;
}


void delay(uint32_t delay){
	uint32_t tick_start = get_tick();
	uint32_t wait = delay;
	if(wait <MAX_DELAY){
		wait += (uint32_t)(tick_freq);
	}
	
	while((get_tick()- tick_start) < wait){}
}	
void SysTick_Handler(){
    g_curr_tick += tick_freq; //
	SCB_ICSR |= SCB_ICSR_PENDSVSET;
	
}