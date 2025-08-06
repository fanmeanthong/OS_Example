#ifndef __KERNEL_H // 
#define __KERNEL_H

#include <stdint.h>
#include "uart.h"

// =====================
// Type Definitions
// =====================
/**
 * @brief Tick count type (usually uint16 or uint32)
 */
typedef uint32_t TickType;
/**
 * @brief Alarm identifier type
 */
typedef uint32_t AlarmTypeId;
/**
 * @brief Counter identifier type
 */
typedef uint32_t CounterTypeId;
/**
 * @brief Task identifier type
 */
typedef uint32_t TaskType;
/**
 * @brief Event mask type for Extended Task
 */
typedef uint32_t EventMaskType;

// =====================
// State and Action Enums
// =====================
/**
 * @brief Alarm state (active/inactive)
 */
typedef enum {
    ALARMSTATE_INACTIVE = 0,
    ALARMSTATE_ACTIVE
} AlarmStateType;
/**
 * @brief Alarm action type (activate task, set event, callback)
 */
typedef enum {
    ALARMACTION_ACTIVATETASK,
    ALARMACTION_SETEVENT,
    ALARMACTION_CALLBACK
} AlarmActionType;
/**
 * @brief Task state (suspended, ready, running, waiting)
 */
typedef enum {
    SUSPENDED,
    READY,
    RUNNING,
    WAITING
} TaskStateType;

// =====================
// System Limits
// =====================
#define TASK_NUM 3        // Number of tasks in the system
#define MAX_ALARMS 10     // Maximum number of alarms per counter
#define MAX_SCHEDULETABLES 4 // Maximum number of schedule tables
#define MAX_EXPIRY_POINTS  8 // Maximum number of expiry points per schedule table

// =====================
// Error Codes
// =====================
#define E_OK       0      // No error
#define E_OS_LIMIT 1      // Limit error (e.g. activation limit)

// =====================
// Struct Definitions
// =====================
/**
 * @brief Structure representing the control block for a task in the operating system.
 *
 * Holds all relevant information about a task, including its identifier,
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

/**
 * @brief Structure representing an alarm in the OS kernel.
 *
 * - state: Current state of the alarm (ACTIVE/INACTIVE).
 * - expiry_tick: Tick value when the alarm expires.
 * - cycle: Repeat interval; 0 for one-shot alarms.
 * - action_type: Type of action to perform on expiry.
 * - action: Action details (task activation, event setting, or callback).
 */
typedef struct {
    AlarmStateType state;              // ACTIVE / INACTIVE
    TickType expiry_tick;              // Expiry time (counter value)
    TickType cycle;                    // Repeat cycle, 0 for one-shot
    AlarmActionType action_type;       // Action type
    union {
        TaskType task_id;              // For ACTIVATE_TASK
        struct {
            TaskType task_id;
            EventMaskType event;
        } set_event;                   // For SET_EVENT
        void (*callback_fn)(void);     // For CALLBACK
    } action;
} AlarmType;

/**
 * @brief Structure representing a hardware/software counter.
 *
 * - current_value: Current tick value of the counter.
 * - max_allowed_value: Maximum tick value before wrap-around.
 * - ticks_per_base: Number of real ticks per counter increment.
 * - min_cycle: Minimum allowed cycle for alarms.
 * - alarm_list: Array of pointers to alarms attached to this counter.
 * - num_alarms: Number of alarms attached.
 */ 
typedef struct {
    TickType current_value;          // Current value of the counter
    TickType max_allowed_value;      // Maximum value before wrap-around
    TickType ticks_per_base;         // Number of real ticks per counter increment
    TickType min_cycle;              // Minimum cycle for alarms
    AlarmType *alarm_list[MAX_ALARMS]; // List of alarms attached to this counter
    uint8_t num_alarms;              // Number of attached alarms
} CounterType;


// =====================
// API Prototypes
// =====================
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
uint8_t SetAbsAlarm(AlarmTypeId alarm_id, TickType start, TickType cycle);
uint8_t CancelAlarm(AlarmTypeId alarm_id);
uint8_t GetAlarm(AlarmTypeId alarm_id, TickType *tick_left) ;
void SetupAlarm_Demo();
void my_callback();

#endif // __KERNEL_H