#ifndef __KERNEL_H // 
#define __KERNEL_H

#include <stdint.h>
#include "timebase.h"
#include "uart.h"
#define TASK_NUM 3  /* Số Task trong hệ thống */
// ====== TCB Structure ======

typedef uint32_t EventMaskType;
/* Định nghĩa kiểu Task và trạng thái */
typedef uint8_t TaskType;
typedef enum {
    SUSPENDED,
    READY,
    RUNNING,
    WAITING
} TaskStateType;

/* Mã lỗi */
#define E_OK       0
#define E_OS_LIMIT 1

/* Task Control Block: bảng tĩnh quản lý Task */
typedef struct {
    TaskType id;                   /* ID của Task */
    void (*TaskEntry)(void);       /* Con trỏ hàm entry */
    TaskStateType state;           /* Trạng thái hiện tại */
    uint8_t priority;              /* Ưu tiên (0 = cao nhất) */
    uint8_t ActivationCount;       /* Đếm số lần đã activate */
    uint8_t OsTaskActivation;      /* Giới hạn activate */

    EventMaskType SetEventMask;   // sự kiện đã đến
    EventMaskType WaitEventMask;  // sự kiện đang chờ
} TaskControlBlock;

/* Prototype API */
void     OS_Init(void);
uint8_t  ActivateTask(TaskType id);
uint8_t  TerminateTask(void);
uint8_t  ChainTask(TaskType id);
uint8_t  GetTaskState(TaskType id, TaskStateType *state);
void     OS_Schedule(void);

void WaitEvent(EventMaskType mask);
void SetEvent(uint8_t task_id, EventMaskType mask);
void ClearEvent(EventMaskType mask);
EventMaskType GetEvent(TaskType id, EventMaskType *event);
#endif //