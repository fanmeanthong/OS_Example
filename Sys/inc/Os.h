#ifndef OS_H
#define OS_H

#include "stdint.h"
#include "uart.h"
#include "Os_Cfg.h"
/* ===== Định nghĩa TrustedFunction ===== */
typedef uint8_t TrustedFunctionIndexType;
typedef void* TrustedFunctionParameterRefType;

typedef void (*TrustedFunctionType)(TrustedFunctionParameterRefType);

/* ===== API chính ===== */
uint8_t CallTrustedFunction(TrustedFunctionIndexType FunctionIndex,
                               TrustedFunctionParameterRefType FunctionParams);
static void Trusted_LogWrite(void* param);
static void Trusted_LedCtrl(void* param);
/* ===== Mã lỗi ===== */
#define E_OS_ACCESS  1u
#define E_OK         0u
#define TF_LOG_WRITE   0
#define TF_LED_CTRL    1

void Task_User(void);
void Task_Admin(void);
void Task_App2(void);

#endif /* OS_H */
