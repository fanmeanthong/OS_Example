#include "os_hooks.h"
#include "uart.h"   // dùng print_str, print_dec, print_hex
#include <stdarg.h>

/* ===== Weak logging ===== */
__attribute__((weak))
void OS_Log(const char *fmt, ...) {
    (void)fmt;
    print_str("[OS_Log] called\r\n");
}

__attribute__((weak))
void ShutdownOS(StatusType Error) {
    print_str("[ShutdownOS] System halted, error=");
    print_dec((int)Error);
    print_str("\r\n");
    while (1);
}

/* ===== Helper: Print Task Info ===== */
/* ===== Helper: Print Task Info ===== */
static void print_task_info(TaskType id) {
    print_str("[Task] ID=");
    print_dec((int)id);

    print_str(" state=");
    switch (TaskTable[id].state) {
        case SUSPENDED:
            print_str("SUSPENDED");
            break;
        case READY:
            print_str("READY");
            break;
        case RUNNING:
            print_str("RUNNING");
            break;
        case WAITING:
            print_str("WAITING");
            break;
        default:
            print_str("UNKNOWN");
            break;
    }

    print_str("\r\n");
}


/* ===== Hook default implementations (weak) ===== */
#if OS_USE_STARTUPHOOK
__attribute__((weak))
void StartupHook(void) {
    print_str("[Hook] StartupHook()\r\n");
}
#endif

#if OS_USE_SHUTDOWNHOOK
__attribute__((weak))
void ShutdownHook(StatusType e) {
    print_str("[Hook] ShutdownHook err=");
    print_dec((int)e);
    print_str("\r\n");
}
#endif

#if OS_USE_ERRORHOOK
__attribute__((weak))
void ErrorHook(StatusType e) {
    switch (e) {
        case E_OS_ID:
            print_str("[ERROR] Invalid Task ID in ActivateTask()\r\n");
            break;
        case E_OS_STACKFAULT:
            print_str("[ERROR] Stack overflow detected!\r\n");
            break;
        default:
            print_str("[ERROR] OS returned error code: ");
            print_dec((int)e);
            print_str("\r\n");
            break;
    }
}
#endif

#if OS_USE_PRETASKHOOK
__attribute__((weak))
void PreTaskHook(void) {
    print_str("[Hook] PreTaskHook ");
    print_task_info(currentTask);   // chỉ in ID + state
}
#endif

#if OS_USE_POSTTASKHOOK
__attribute__((weak))
void PostTaskHook(void) {
    print_str("[Hook] PostTaskHook ");
    if (OS_StackGuard_Check() != E_OK) {
        print_str("[Hook] PostTaskHook detected stack fault\r\n");
    }
    // In stack usage ở PostTaskHook
}
#endif


/* ===== ActivateTask with validation and ErrorHook ===== */
uint8_t ActivateTask_Hook(TaskType id) {
    StatusType st = OS_Check_InvalidActivateTask(id);
    if (st != E_OK) {
#if OS_USE_ERRORHOOK
        ErrorHook(st);
#endif
        return (uint8_t)st;
    }
    st = (StatusType)ActivateTask(id);
    if (st != E_OK) {
#if OS_USE_ERRORHOOK
        ErrorHook(st);
#endif
    }
    return (uint8_t)st;
}

/* ===== Stack Guard ===== */
static uintptr_t g_guard_low = 0;
static uint32_t  g_guard_margin = 0;
extern uint32_t _ebss;

void OS_StackGuard_Set(void *low_addr, uint32_t margin_bytes) {
    if (low_addr) {
        g_guard_low = (uintptr_t)low_addr;
    } else {
        g_guard_low = (uintptr_t)&_ebss;
    }
    g_guard_margin = margin_bytes;
}

StatusType OS_StackGuard_Check(void) {
    if (g_guard_low == 0) {
        OS_StackGuard_Set(NULL, 512);
    }
    uint32_t msp = OS_GetMSP();
    uintptr_t limit = g_guard_low + g_guard_margin;
    if ((uintptr_t)msp < limit) {
#if OS_USE_ERRORHOOK
        ErrorHook(E_OS_STACKFAULT);
#endif
        ShutdownOS(E_OS_STACKFAULT);
        return E_OS_STACKFAULT;
    }
    return E_OK;
}

/* ===== Stack Watermark ===== */
void OS_StackWatermark_InitRegion(uint8_t *base, uint32_t size_bytes, uint8_t pattern) {
    if (!base || size_bytes == 0) return;
    for (uint32_t i = 0; i < size_bytes; ++i) base[i] = pattern;
}

uint32_t OS_StackWatermark_UsedBytes(uint8_t *base, uint32_t size_bytes, uint8_t pattern) {
    if (!base || size_bytes == 0) return 0;
    uint32_t used = 0;
    for (int32_t i = (int32_t)size_bytes - 1; i >= 0; --i) {
        if (base[i] != pattern) {
            used = (uint32_t)(i + 1);
            break;
        }
    }
    return used;
}

uint8_t OS_StackWatermark_Overflowed(uint8_t *base, uint32_t size_bytes, uint8_t pattern) {
    if (!base || size_bytes == 0) return 0;
    return base[0] != pattern;
}

/* ===== Run task with hooks ===== */
void OS_RunTaskWithHooks(TaskType id) {
    if (id >= (TaskType)TASK_NUM) {
#if OS_USE_ERRORHOOK
        ErrorHook(E_OS_ID);
#endif
        return;
    }
#if OS_USE_PRETASKHOOK
    PreTaskHook();
#endif
    TaskTable[id].TaskEntry();
#if OS_USE_POSTTASKHOOK
    PostTaskHook();
#endif
}
