#ifndef __KERNEL_H // 
#define __KERNEL_H

#include <stdint.h>
#include "timebase.h"
#include "uart.h"

typedef uint32_t TickType;      // Số tick (thường là uint16 hoặc uint32)
typedef uint32_t AlarmTypeId;   // Chỉ số định danh Alarm
typedef uint32_t CounterTypeId; // Chỉ số định danh Counter
typedef uint32_t TaskType;      // ID của Task
typedef uint32_t EventMaskType; // Mask dùng để gán Event cho Extended Task
typedef enum {
    ALARMSTATE_INACTIVE = 0,
    ALARMSTATE_ACTIVE
} AlarmStateType;

typedef enum {
    ALARMACTION_ACTIVATETASK,
    ALARMACTION_SETEVENT,
    ALARMACTION_CALLBACK
} AlarmActionType;
#define TASK_NUM 3  /* Số Task trong hệ thống */
#define MAX_ALARMS 10 // Số lượng Alarm tối đa cho mỗi Counter
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

typedef struct {
    AlarmStateType state;              // ACTIVE / INACTIVE
    TickType expiry_tick;              // Thời điểm kích hoạt (Counter value)
    TickType cycle;                    // Chu kỳ lặp lại, 0 nếu chỉ chạy 1 lần
    AlarmActionType action_type;       // Loại hành động

    union {
        TaskType task_id;              // Nếu là ACTIVATE_TASK
        struct {
            TaskType task_id;
            EventMaskType event;
        } set_event;                   // Nếu là SET_EVENT
        void (*callback_fn)(void);     // Nếu là CALLBACK
    } action;
} AlarmType;

typedef struct {
    TickType current_value;          // Giá trị hiện tại của Counter
    TickType max_allowed_value;      // Giá trị tối đa trước khi quay vòng về 0
    TickType ticks_per_base;         // Bao nhiêu tick thực tế mới tăng 1 đơn vị
    TickType min_cycle;              // Chu kỳ nhỏ nhất cho Alarm
    AlarmType *alarm_list[MAX_ALARMS]; // Danh sách các Alarm gắn vào Counter này
    uint8_t num_alarms;              // Số lượng alarm đang gắn
} CounterType;

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