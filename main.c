#include <stdint.h>
#include "stm32f10x.h"
#include "kernel.h"
#include "uart.h"
#include "setup.h"
#include "os_hooks.h"
#include <stdio.h>
/**
 * @brief Global variable for current LED mode
 */
LedMode g_mode;
// =====================
// Schedule Table Callbacks
// =====================
// Callbacks for Schedule Table
void SetMode_Normal(void)  { g_mode = MODE_NORMAL; }
void SetMode_Warning(void) { g_mode = MODE_WARNING; }
void SetMode_Off(void)     { g_mode = MODE_OFF; }


// --- Led control task ---
void Task_LedTick(void) {
    static uint16_t accA = 0, accC = 0;
    const uint16_t period_normal = 500;
    const uint16_t period_warn   = 100;

    switch (g_mode) {
    case MODE_NORMAL:
        accA += 50;
        if (accA >= period_normal) {
            LedA_Toggle();
            accA = 0;
        }
        break;
    case MODE_WARNING:
        accC += 50;
        if (accC >= period_warn) {
            Led_Toggle();
            accC = 0;
        }
        break;
    default:
        LedA_Off();
        Led_Off();
        accA = accC = 0;
        break;
    }
    TerminateTask();
}

// =====================
// External Variables
// =====================
extern TaskControlBlock TaskTable[TASK_NUM];
extern TaskType   currentTask ;

// =====================
// Event & Task Macros
// =====================
/*
#define EVENT_BTN_PRESS      (1U << 0)
#define TASK_LED_CTRL_ID     0
#define TASK_BLINK_ID        1
#define TASK_BTN_POLL_ID     2
*/
// =====================
// SysTick Functions
// =====================
/**
 * @brief Initialize SysTick timer for 1ms interrupt
 */
void SysTick_Init(void) {
    // PendSV priority = 0xFF, SysTick = 0xFE
    uint32_t v = SCB_SHPR3;
    v &= ~0xFFFF0000u;
    v |= (0xFFu << 16) | (0xFEu << 24);
    SCB_SHPR3 = v;

    SYST_RVR = SYSTICK_LOAD_VAL;
    SYST_CVR = 0;
    SYST_CSR = SYSTICK_CTRL_ENABLE | SYSTICK_CTRL_TICKINT | SYSTICK_CTRL_CLKSRC;
}

/**
 * @brief SysTick interrupt handler
 */

void SysTick_Handler(void) {
    SCB_ICSR = ICSR_PENDSVSET;   
}

void PendSV_Handler(void) {
    Counter_Tick(0);
    OS_Schedule();
}
// =====================
// Utility Functions
// =====================
/**
 * @brief Delay in milliseconds (approximate)
 */
void delay_ms(volatile uint32_t ms) {
    for (volatile uint32_t i = 0; i < ms * 8000; i++) {
        __asm volatile ("nop");
    }
}

// =====================
// System Initialization
// =====================
/**
 * @brief Configure system clock to 72MHz using PLL
 */
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

/**
 * @brief Initialize GPIO for LED and button
 */
void GPIO_InitAll(void) {
    // RCC Enable GPIOA và GPIOC
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

    GPIO_ODR(GPIOA_BASE) |= (1 << 1);         // Pull-Up
}

// =====================
// LED Control Functions
// =====================
/**
 * @brief Turn on LED at PC13
 */
void Led_Toggle(void) {
    GPIO_ODR(GPIOC_BASE) ^= (1 << 13); // Toggle PC13
}
void Led_On (void) {  GPIO_ODR(GPIOC_BASE)  = !(1 << 13); }
/**
 * @brief Turn off LED at PC13
 */
void Led_Off(void) {  GPIO_ODR(GPIOC_BASE) = (1 << 13); }
/**
 * @brief Turn on LED at PA0
 */
void LedA_On(void) { GPIO_ODR(GPIOA_BASE) |= (1 << 0); }
/**
 * @brief Turn off LED at PA0
 */
void LedA_Off(void) { GPIO_ODR(GPIOA_BASE) &= ~(1 << 0); }
/**
 * @brief Toggle LED at PA0
 */
void LedA_Toggle(void) { GPIO_ODR(GPIOA_BASE) ^= (1 << 0); }

/**
 * @brief Blink LED once and terminate task
 */
void Task_Blink(void) {
    Led_On();  delay_ms(10);
    Led_Off(); delay_ms(10);
    TerminateTask();  
}

/**
 * @brief Poll button state and set event if pressed

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
 */
/**
 * @brief Control LED when button event occurs

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
 */
void Task_SpeedSensor(void) {
    int speed = 60 + (rand() % 20);
    print_str("[Sensor] Send speed=");
    print_dec(speed);   
    print_str(" km/h\r\n");
    IocSend(IOC_CH_SPEED, &speed);
    ChainTask(TASK_SENSOR_ID);
}

void Task_Cluster(void) {
    int speed;
    if (IocHasNewData(IOC_CH_SPEED, TASK_CLUSTER_ID)) {
        IocReceive(IOC_CH_SPEED, &speed, TASK_CLUSTER_ID);
        print_str("[Cluster] speed=");
        print_dec(speed);
        print_str(" km/h\r\n");
    }
    ChainTask(TASK_CLUSTER_ID);
}

void Task_ABS(void) {
    int speed;
    if (IocHasNewData(IOC_CH_SPEED, TASK_ABS_ID)) {
        IocReceive(IOC_CH_SPEED, &speed, TASK_ABS_ID);
        print_str("[ABS] speed=");
        print_dec(speed);
        print_str(" km/h\r\n");
    }
    ChainTask(TASK_ABS_ID);
}

// =====================
// Main Function
// =====================
/**
 * @brief Main entry: system initialization, task setup, and scheduler loop
 */
int main(void) {
    SystemClock_Config();
    GPIO_InitAll();
    UART1_Init();

    print_str("=== IOC 1-1 Demo Start ===\r\n");

    OS_Init();

    TaskType recv_list_speed[2] = { TASK_CLUSTER_ID, TASK_ABS_ID };
    Ioc_InitChannel(IOC_CH_SPEED, sizeof(int), recv_list_speed, 2);



    // --- Tạo bảng task ---
    TaskTable[TASK_SENSOR_ID] = (TaskControlBlock){ 
        TASK_SENSOR_ID, Task_SpeedSensor, SUSPENDED, 0, 2};

    TaskTable[TASK_CLUSTER_ID] = (TaskControlBlock){ 
        TASK_CLUSTER_ID, Task_Cluster, SUSPENDED, 0, 2};

    TaskTable[TASK_ABS_ID] = (TaskControlBlock){ 
        TASK_ABS_ID, Task_ABS, SUSPENDED, 0 ,2};

    // 
    ActivateTask(TASK_SENSOR_ID);
    ActivateTask(TASK_CLUSTER_ID);
    ActivateTask(TASK_ABS_ID);

    while (1) {
        OS_Schedule();
    }
}
