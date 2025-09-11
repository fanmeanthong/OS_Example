#include "Os.h"
#include "stm32f10x.h"
#include "kernel.h"
#include "uart.h"
#include "setup.h"

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
/* Task Trusted: được phép ghi log & điều khiển LED */
void Task_Admin(void) {
    print_str(">>>>> Enter Admin Task -------\r\n");

    const char* msg = "System started by Admin";
    int ledState = 1;

    CallTrustedFunction(TF_LOG_WRITE, (void*)msg);      // OK
    CallTrustedFunction(TF_LED_CTRL, (void*)&ledState); // OK bật LED

    ledState = 0;
    CallTrustedFunction(TF_LED_CTRL, (void*)&ledState); // OK tắt LED

    print_str("<<<<< Exit Admin Task --------\r\n");
    print_str("\r\n");
    TerminateTask();
}

/* Task Untrusted: bị chặn */
void Task_User(void) {
    print_str(">>>>> Enter User Task -------\r\n");

    const char* msg = "User try to write log";
    int ledState = 1;

    CallTrustedFunction(TF_LOG_WRITE, (void*)msg);      // Bị OS chặn
    CallTrustedFunction(TF_LED_CTRL, (void*)&ledState); // Bị OS chặn

    print_str("<<<<< Exit User Task --------\r\n");
    print_str("\r\n");
    TerminateTask();
}

void Task_App2(void) {
    print_str(">>>>> Enter App2 Task -------\r\n");

    const char* msg = "App2 write log";
    int ledState = 1;

    /* Được phép: TF0 */
    CallTrustedFunction(TF_LOG_WRITE, (void*)msg);

    /* Không được phép: TF1 */
    CallTrustedFunction(TF_LED_CTRL, (void*)&ledState);
    
    print_str("<<<<< Exit App2 Task --------\r\n");
    print_str("\r\n");
    TerminateTask();
}