#ifndef __OS_HOOKS_H__
#define __OS_HOOKS_H__

#include <stdint.h>
#include <stddef.h>
#include "kernel.h"
#include "Os_Cfg.h"   /* Hook enable macros from config */

/* ===== Types & Error Codes ===== */
#ifndef StatusType
typedef uint32_t StatusType;
#endif

#ifndef E_OS_STACKFAULT
#define E_OS_STACKFAULT  0x100u
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* ====== Hook Prototypes (guarded by macros) ====== */
#if OS_USE_STARTUPHOOK
void StartupHook(void);
#endif

#if OS_USE_SHUTDOWNHOOK
void ShutdownHook(StatusType Error);
#endif

#if OS_USE_ERRORHOOK
void ErrorHook(StatusType Error);
#endif

#if OS_USE_PRETASKHOOK
void PreTaskHook(void);
#endif

#if OS_USE_POSTTASKHOOK
void PostTaskHook(void);
#endif

/* ====== Optional OS services ====== */
void ShutdownOS(StatusType Error);
void OS_Log(const char *fmt, ...);

/* ====== Task table từ kernel.c ====== */
extern TaskControlBlock TaskTable[TASK_NUM];
extern TaskType currentTask;

/* ====== ID check helper ====== */
static inline StatusType OS_Check_InvalidActivateTask(TaskType id) {
    return (id >= (TaskType)TASK_NUM) ? (StatusType)E_OS_ID : (StatusType)E_OK;
}

/* Wrapper for ActivateTask with validation & ErrorHook */
uint8_t ActivateTask_Hook(TaskType id);

/* ====== Stack Guard ====== */
void     OS_StackGuard_Set(void *low_addr, uint32_t margin_bytes);
StatusType OS_StackGuard_Check(void);
static inline uint32_t OS_GetMSP(void) {
    uint32_t r;
    __asm volatile ("MRS %0, msp" : "=r"(r) :: );
    return r;
}

/* ====== Stack Watermark ====== */
void     OS_StackWatermark_InitRegion(uint8_t *base, uint32_t size_bytes, uint8_t pattern);
uint32_t OS_StackWatermark_UsedBytes(uint8_t *base, uint32_t size_bytes, uint8_t pattern);
uint8_t  OS_StackWatermark_Overflowed(uint8_t *base, uint32_t size_bytes, uint8_t pattern);

/* ====== Run one task with hooks ====== */
void OS_RunTaskWithHooks(TaskType id);

#ifdef __cplusplus
}
#endif

#endif /* __OS_HOOKS_H__ */
