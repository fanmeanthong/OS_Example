#include <stdint.h>
#include "stm32f10x.h"
#include "kernel.h"
#include "timebase.h"
#include "uart.h"
extern TaskControlBlock TaskTable[TASK_NUM];
extern TaskType    currentTask ;
#define FLASH_BASE           ((uint32_t)0x40022000)
#define FLASH_ACR            (*(volatile uint32_t *)(FLASH_BASE + 0x00))

#define RCC_CR               (*(volatile uint32_t *)(RCC_BASE + 0x00))
#define RCC_CFGR             (*(volatile uint32_t *)(RCC_BASE + 0x04))

#define FLASH_ACR_PRFTBE     (1 << 4)
#define FLASH_ACR_LATENCY_2  (2 << 0)
#define FLASH_ACR_LATENCY    (0x7 << 0)

#define RCC_CR_HSEON         (1 << 16)
#define RCC_CR_HSERDY        (1 << 17)
#define RCC_CR_PLLON         (1 << 24)
#define RCC_CR_PLLRDY        (1 << 25)

#define RCC_CFGR_SW          (0x3 << 0)
#define RCC_CFGR_SW_PLL      (0x2 << 0)
#define RCC_CFGR_SWS         (0x3 << 2)
#define RCC_CFGR_SWS_PLL     (0x2 << 2)

#define RCC_CFGR_HPRE        (0xF << 4)
#define RCC_CFGR_PPRE1       (0x7 << 8)
#define RCC_CFGR_PPRE2       (0x7 << 11)

#define RCC_CFGR_PPRE1_DIV2  (0x4 << 8)

#define RCC_CFGR_PLLSRC      (1 << 16)
#define RCC_CFGR_PLLMULL     (0xF << 18)
#define RCC_CFGR_PLLMULL9    (0x7 << 18)

#define GPIO_CRL(GPIO_BASE)  (*(volatile uint32_t *)((GPIO_BASE) + GPIO_CRL_OFFSET))
#define GPIO_CRH(GPIO_BASE)  (*(volatile uint32_t *)((GPIO_BASE) + GPIO_CRH_OFFSET))
#define GPIO_ODR(GPIO_BASE)  (*(volatile uint32_t *)((GPIO_BASE) + GPIO_ODR_OFFSET))
#define GPIO_IDR(GPIO_BASE)   (*(volatile uint32_t *)((GPIO_BASE) + GPIO_IDR_OFFSET))

#define EVENT_BTN_PRESS   (1U << 0)

#define TASK_LED_CTRL_ID   0
#define TASK_BLINK_ID      1
#define TASK_BTN_POLL_ID   2

void delay_ms(volatile uint32_t ms) {
    for (volatile uint32_t i = 0; i < ms * 8000; i++) {
        __asm volatile ("nop");
    }
}
void SystemClock_Config(void)
{
    // Enable HSE
    RCC_CR |= RCC_CR_HSEON;
    while (!(RCC_CR & RCC_CR_HSERDY));

    // Prefetch + 2 WS for 72MHz
    FLASH_ACR |= FLASH_ACR_PRFTBE;
    FLASH_ACR &= ~FLASH_ACR_LATENCY;
    FLASH_ACR |= FLASH_ACR_LATENCY_2;

    // PLL config: HSE source, x9
    RCC_CFGR &= ~(RCC_CFGR_PLLSRC | RCC_CFGR_PLLMULL);
    RCC_CFGR |= (RCC_CFGR_PLLSRC | RCC_CFGR_PLLMULL9);

    // Prescaler
    RCC_CFGR &= ~RCC_CFGR_HPRE;
    RCC_CFGR &= ~RCC_CFGR_PPRE1;
    RCC_CFGR |= RCC_CFGR_PPRE1_DIV2;
    RCC_CFGR &= ~RCC_CFGR_PPRE2;

    // Enable PLL
    RCC_CR |= RCC_CR_PLLON;
    while (!(RCC_CR & RCC_CR_PLLRDY));

    // Switch SYSCLK to PLL
    RCC_CFGR &= ~RCC_CFGR_SW;
    RCC_CFGR |= RCC_CFGR_SW_PLL;
    while ((RCC_CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);
}

// ====== Scheduler Launch Function ======
void GPIO_InitAll(void) {
    // Bật xung cho GPIOA và GPIOC
    RCC_APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPCEN;

    // PC13: Output Push-Pull 2MHz (LED nháy)
    GPIO_CRH(GPIOC_BASE) &= ~(0xF << 20);
    GPIO_CRH(GPIOC_BASE) |=  (0x2 << 20);

    // PA0: Output Push-Pull 2MHz 
    GPIO_CRL(GPIOA_BASE) &= ~(0xF << 0);      // Clear CNF0 + MODE0
    GPIO_CRL(GPIOA_BASE) |=  (0x2 << 0);      // MODE0 = 10 (2MHz), CNF0 = 00 (Push-Pull)

    // PA1: Input Pull-Up (MODE1 = 00, CNF1 = 10)
    GPIO_CRL(GPIOA_BASE) &= ~(0xF << 4);      // Clear CNF1[1:0] + MODE1[1:0]
    GPIO_CRL(GPIOA_BASE) |=  (0x8 << 4);      // CNF1 = 10 (Input Pull-Up), MODE1 = 00

    GPIO_ODR(GPIOA_BASE) |= (1 << 1);         // Kéo lên (bắt buộc cho Pull-Up)
}

void Led_On (void) {  GPIO_ODR(GPIOC_BASE)  = !(1 << 13); }
void Led_Off(void) {  GPIO_ODR(GPIOC_BASE) = (1 << 13); }
void LedA_On(void) { GPIO_ODR(GPIOA_BASE) |= (1 << 0); }
void LedA_Off(void) { GPIO_ODR(GPIOA_BASE) &= ~(1 << 0); }
void LedA_Toggle(void) { GPIO_ODR(GPIOA_BASE) ^= (1 << 0); }
/* --- Triển khai 3 Task tuần tự --- */

void Task_Blink(void) {
    LedA_On();  delay_ms(200);
    LedA_Off(); delay_ms(200);
    
    ChainTask(TASK_BLINK_ID);  // 
}

void Task_ButtonPoll(void) {
    static uint8_t last_state = 1;
    uint8_t now = (GPIO_IDR(GPIOA_BASE) & (1 << 1)) ? 1 : 0;

    if (last_state == 1 && now == 0) {
        delay_ms(20);  // debounce
        if ((GPIO_IDR(GPIOA_BASE) & (1 << 1)) == 0) {
            SetEvent(TASK_LED_CTRL_ID, EVENT_BTN_PRESS);
        }
    }
    last_state = now;
    ChainTask(TASK_BTN_POLL_ID);  // 
}

void Task_LEDControl(void) {
    
    EventMaskType ev;
    WaitEvent(EVENT_BTN_PRESS);
    GetEvent(currentTask, &ev);

    if (ev & EVENT_BTN_PRESS) {
        Led_On();  
        delay_ms(500);
        Led_Off();
        delay_ms(500);
        ClearEvent(EVENT_BTN_PRESS);
    }
    ActivateTask(TASK_LED_CTRL_ID);
    TerminateTask();
}

int main(void) {
    SystemClock_Config();
    GPIO_InitAll();
    OS_Init();

    // Khởi tạo Task Table
    TaskTable[TASK_LED_CTRL_ID] = (TaskControlBlock){TASK_LED_CTRL_ID, Task_LEDControl, SUSPENDED, 1, 0, 2};
    TaskTable[TASK_BLINK_ID]    = (TaskControlBlock){TASK_BLINK_ID,    Task_Blink,      SUSPENDED, 1, 0, 2};
    TaskTable[TASK_BTN_POLL_ID] = (TaskControlBlock){TASK_BTN_POLL_ID, Task_ButtonPoll, SUSPENDED, 1, 0, 2};

    // Khởi động ban đầu các task
    ActivateTask(TASK_BTN_POLL_ID);  // Task này luôn polling → nên chạy trước
    ActivateTask(TASK_BLINK_ID);     // Nháy LED liên tục
    ActivateTask(TASK_LED_CTRL_ID);  // Sẽ đợi sự kiện

    while (1) {
        OS_Schedule();
    }
}