#include "Os.h"
#include "kernel.h"
#include <stdio.h>

/* === Trusted Function prototypes === */
static void Trusted_LogWrite(void* param);
static void Trusted_LedCtrl(void* param);

/* === Bảng đăng ký TrustedFunction === */
static const TrustedFunctionType s_trustedFunctions[] = {
    Trusted_LogWrite,   /* ID = 0 */
    Trusted_LedCtrl     /* ID = 1 */
};

/* === Cờ phân quyền cho từng App ===
 *   App0 (Trusted): có quyền gọi cả TF0, TF1
 *   App1 (Untrusted): không có quyền
 */
static const uint8_t s_trustedPermissions[][2] = {
    /* App0 */ {1, 1},  // có quyền gọi cả TF0, TF1
    /* App1 */ {0, 0}   // không có quyền gọi gì
};

/* === Biến mô phỏng App hiện tại (runtime) === */
static uint8_t CurrentAppID = 0;  // 0=Trusted, 1=Untrusted
/* Số lượng TF tối đa */
#define TF_COUNT   (sizeof(s_trustedFunctions)/sizeof(s_trustedFunctions[0]))

extern const TrustedFunctionType s_trustedFunctions[];
extern const uint8_t s_trustedPermissions[][2];
extern TaskControlBlock TaskTable[];
extern TaskType currentTask;

uint8_t CallTrustedFunction(TrustedFunctionIndexType FunctionIndex,
                            TrustedFunctionParameterRefType FunctionParams)
{
    uint8_t appid = TaskTable[currentTask].AppID;  // lấy AppID runtime

    if (FunctionIndex >= TF_COUNT) {
        return E_OS_ACCESS;
    }
    if (s_trustedPermissions[appid][FunctionIndex] == 0) {
        print_str("[OS] App");
        print_dec(appid);
        print_str(" cannot call TF");
        print_dec(FunctionIndex);
        print_str("\r\n");
        return E_OS_ACCESS;
    }

    s_trustedFunctions[FunctionIndex](FunctionParams);
    return E_OK;
}

/* === Trusted function: Ghi log === */
static void Trusted_LogWrite(void* param)
{
    const char* msg = (const char*)param;
    print_str("[TF-LOG] ");
    print_str(msg); 
    print_str("\r\n");
}

/* === Trusted function: Điều khiển LED === */
static void Trusted_LedCtrl(void* param)
{
    int* state = (int*)param;
    if (*state)
        print_str("[TF-LED] LED ON (Risky)\r\n");
    else
        print_str("[TF-LED] LED OFF\r\n");
}