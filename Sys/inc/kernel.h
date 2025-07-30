#ifndef __KERNEL_H // 
#define __KERNEL_H

#include <stdint.h>
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

/* Định nghĩa trạng thái */
typedef enum {
    SUSPENDED,
    READY,
    RUNNING,
    WAITING
} TaskStateType;

/* Mã lỗi */
#define E_OK       0
#define E_OS_LIMIT 1

/**
 * @brief Structure representing the control block for a task in the operating system.
 *
 * This structure holds all relevant information about a task, including its identifier,
 * entry function, current state, priority, activation count, activation limit, and event masks.
 */
typedef struct {
    TaskType id;                   // Task ID
    void (*TaskEntry)(void);       // Pointer to task entry function
    TaskStateType state;           // Current state of the task
    uint8_t priority;              // Task priority (0 = highest)
    uint8_t ActivationCount;       // Number of times the task has been activated
    uint8_t OsTaskActivation;      // Activation limit for the task

    EventMaskType SetEventMask;    // Events that have been set (arrived)
    EventMaskType WaitEventMask;   // Events the task is currently waiting for
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


void Counter_Tick(CounterTypeId cid);
uint8_t SetRelAlarm(AlarmTypeId alarm_id, TickType offset, TickType cycle);
uint8_t SetAbsAlarm(AlarmTypeId alarm_id, TickType start, TickType);
uint8_t CancelAlarm(AlarmTypeId alarm_id);
void SetupAlarm_Demo() ;
void my_callback();
#endif //