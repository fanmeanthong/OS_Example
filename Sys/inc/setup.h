#ifndef __SETUP_H__
#define __SETUP_H__
#include <stdint.h>
// =====================
// LED Mode Enum & Global
// =====================
/**
 * @brief LED operation modes
 */
typedef enum { MODE_NORMAL = 0, MODE_WARNING, MODE_OFF } LedMode;
// =====================
// Function Prototypes
// =====================
void SysTick_Init(void);
void SysTick_Handler(void);
void delay_ms(volatile uint32_t ms);
void SystemClock_Config(void);
void GPIO_InitAll(void);
void Led_Toggle(void);
void Led_On(void);
void Led_Off(void);
void LedA_On(void);
void LedA_Off(void);
void LedA_Toggle(void);
void Task_Blink(void);
void Task_ButtonPoll(void);
void Task_LEDControl(void);
// =====================
// Task IDs
// =====================
#define TASK_LED_TICK_ID   0  // Task ID for LED tick task
void Task_LedTick(void);
void SetMode_Normal(void);
void SetMode_Warning(void);
void SetMode_Off(void);
#endif // __SETUP_H__