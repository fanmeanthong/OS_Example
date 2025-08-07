#include "kernel.h"

// =====================
// Global Variables
// =====================
TaskControlBlock TaskTable[TASK_NUM];
TaskType    currentTask = 0;

AlarmType alarm_table[MAX_ALARMS];
CounterType* alarm_to_counter[MAX_ALARMS];

#define COUNTER_NUM 2
CounterType counter_table[COUNTER_NUM] = {
    // Counter 0 - counter_1ms
    {
        .current_value = 0,
        .max_allowed_value = 10000,
        .ticks_per_base = 1,
        .min_cycle = 1,
        .num_alarms = 0
    },
    // Counter 1 - counter_100ms
    {
        .current_value = 0,
        .max_allowed_value = 500,
        .ticks_per_base = 100,
        .min_cycle = 10,
        .num_alarms = 0
    }
};

// =====================
// Schedule Table Variables
// =====================
/**
 * @brief Global array of schedule tables
 */
ScheduleTableType schedule_table_list[MAX_SCHEDULETABLES];
/**
 * @brief Number of schedule tables in use
 */
uint8_t schedule_table_count = 0;

// =====================
// OS Core Functions
// =====================
/**
 * @brief Initialize OS: set all tasks to SUSPENDED, ActivationCount = 0
 */
void OS_Init(void) {
    for (int i = 0; i < TASK_NUM; i++) {
        TaskTable[i].state           = SUSPENDED;
        TaskTable[i].ActivationCount = 0;
        TaskTable[i].OsTaskActivation= 2;
    }
}

/**
 * @brief Activate a task: increase count, set READY if SUSPENDED
 */
uint8_t ActivateTask(TaskType id) {
    TaskControlBlock *t = &TaskTable[id];

    if (t->ActivationCount >= t->OsTaskActivation) {
        return E_OS_LIMIT;
    }

    t->ActivationCount++;

    // SUSPENDED → READY
    if (t->state == SUSPENDED) {
        t->state = READY;
    }

    return E_OK;
}

/**
 * @brief Terminate current task: decrease count, set SUSPENDED or READY
 */
uint8_t TerminateTask(void) {
    TaskControlBlock *t = &TaskTable[currentTask];

    if (t->ActivationCount > 0) {
        t->ActivationCount--;
    }

    // Still activate → READY
    if (t->ActivationCount > 0) {
        t->state = READY;
    } else {
        t->state = SUSPENDED;
    }

    //OS_Schedule();
    return E_OK;
}

/**
 * @brief Chain current task to another: terminate current, activate another
 */
uint8_t ChainTask(TaskType id) {
    ActivateTask(id);        // 
    TerminateTask();         // 
    return E_OK;
}

/**
 * @brief Get current state of a task
 */
uint8_t GetTaskState(TaskType id, TaskStateType *s) {
    *s = TaskTable[id].state;
    return E_OK;
}

/**
 * @brief Cooperative round-robin scheduler: pick first READY task
 */
void OS_Schedule(void) {
    for (int i = 1; i <= TASK_NUM; i++) {
        TaskType idx = (currentTask + i) % TASK_NUM;
        if (TaskTable[idx].state == READY) {
            TaskTable[idx].state   = RUNNING;
            currentTask            = idx;
            TaskTable[idx].TaskEntry();    
            return;
        }
    }
}

// =====================
// Event Functions
// =====================
/**
 * @brief Wait for event mask, set WAITING if not arrived
 */
void WaitEvent(EventMaskType mask) {
    if ((TaskTable[currentTask].SetEventMask & mask) == 0) {
        TaskTable[currentTask].WaitEventMask = mask;
        TaskTable[currentTask].state = WAITING;
        //OS_Schedule();
        return; // 
    }
    // Keep RUNNING
}

/**
 * @brief Set event mask for a task, wake up if waiting
 */
void SetEvent(uint8_t task_id, EventMaskType mask) {
    TaskTable[task_id].SetEventMask |= mask;

    if (TaskTable[task_id].state == WAITING &&
        (TaskTable[task_id].SetEventMask & TaskTable[task_id].WaitEventMask)) {
        
        TaskTable[task_id].state = READY;
        TaskTable[task_id].WaitEventMask = 0;
    }
}

/**
 * @brief Clear event mask for current task
 */
void ClearEvent(EventMaskType mask) {
    TaskTable[currentTask].SetEventMask &= ~mask;
}

/**
 * @brief Get event mask of a task
 */
EventMaskType GetEvent(TaskType id, EventMaskType *event) {
    *event = TaskTable[id].SetEventMask;
    return E_OK;
}

// =====================
// Alarm & Counter Functions
// =====================
/**
 * @brief Set relative alarm: activate after offset, repeat by cycle
 */
uint8_t SetRelAlarm(AlarmTypeId alarm_id, TickType offset, TickType cycle) {
    if (alarm_id >= MAX_ALARMS || offset == 0) return E_OS_LIMIT;

    AlarmType *a = &alarm_table[alarm_id];
    CounterType *c = alarm_to_counter[alarm_id];

    a->state = ALARMSTATE_ACTIVE;
    a->expiry_tick = (c->current_value + offset) % c->max_allowed_value;
    a->cycle = cycle;

    return E_OK;
}

/**
 * @brief Set absolute alarm: activate at start, repeat by cycle
 */
uint8_t SetAbsAlarm(AlarmTypeId alarm_id, TickType start, TickType cycle) {
    if (alarm_id >= MAX_ALARMS) return E_OS_LIMIT;

    AlarmType *a = &alarm_table[alarm_id];
    CounterType *c = alarm_to_counter[alarm_id];

    a->state = ALARMSTATE_ACTIVE;
    a->expiry_tick = start % c->max_allowed_value;
    a->cycle = cycle;

    return E_OK;
}

/**
 * @brief Cancel alarm by ID
 */
uint8_t CancelAlarm(AlarmTypeId alarm_id) {
    if (alarm_id >= MAX_ALARMS) return E_OS_LIMIT;

    AlarmType *a = &alarm_table[alarm_id];
    a->state = ALARMSTATE_INACTIVE;
    return E_OK;
}


/**
 * @brief Tick counter, check and trigger alarms
 */
void Counter_Tick(CounterTypeId cid) {
    CounterType *c = &counter_table[cid];
    c->current_value = (c->current_value + 1) % c->max_allowed_value;

    for (int i = 0; i < c->num_alarms; i++) {
        AlarmType *a = c->alarm_list[i];

        if (a->state == ALARMSTATE_ACTIVE && a->expiry_tick == c->current_value) {
            switch (a->action_type) {
                case ALARMACTION_ACTIVATETASK:
                    ActivateTask(a->action.task_id);
                    break;
                case ALARMACTION_SETEVENT:
                    SetEvent(a->action.set_event.task_id, a->action.set_event.event);
                    break;
                case ALARMACTION_CALLBACK:
                    a->action.callback_fn();
                    break;
            }

            if (a->cycle > 0) {
                a->expiry_tick = (c->current_value + a->cycle) % c->max_allowed_value;
            } else {
                a->state = ALARMSTATE_INACTIVE;
            }
        }
    }
    ScheduleTable_Tick(cid); // Check schedule tables on each counter tick
}

// =====================
// Demo & Utility Functions
// =====================
/**
 * @brief Example callback for alarm
 */
void my_callback() {
    // Callback function example
    LedA_Toggle();  // Toggle LED A
}
void my_callback1() {
    // Callback function example
    Led_Toggle();  // Toggle LED PC13
}
/**
 * @brief Setup demo alarms for testing
 */
void SetupAlarm_Demo() {
    AlarmTypeId alarm_id = 0;
    CounterTypeId counter_id = 0;

    alarm_to_counter[alarm_id] = &counter_table[counter_id];
    counter_table[counter_id].alarm_list[counter_table[counter_id].num_alarms++] = &alarm_table[alarm_id];

    alarm_table[alarm_id].action_type = ALARMACTION_CALLBACK;
    alarm_table[alarm_id].action.callback_fn = my_callback; // Callback function example

    alarm_to_counter[1] = &counter_table[counter_id];
    counter_table[counter_id].alarm_list[counter_table[counter_id].num_alarms++] = &alarm_table[1];

    alarm_table[1].action_type = ALARMACTION_ACTIVATETASK;
    alarm_table[1].action.task_id = 1; // Activate Task Blink

    SetRelAlarm(0, 100, 250);
    SetRelAlarm(1, 200, 5000); 
}

// =====================
// Schedule Table Functions
// =====================

/**
 * @brief Start a schedule table at a relative offset
 */
uint8_t StartScheduleTableRel(uint8_t table_id, TickType offset) {
    if (table_id >= MAX_SCHEDULETABLES || schedule_table_list[table_id].active)
        return E_OS_LIMIT;

    ScheduleTableType *tbl = &schedule_table_list[table_id];
    tbl->start_time = (tbl->counter->current_value + offset) % tbl->counter->max_allowed_value;
    tbl->current_ep = 0;
    tbl->active = 1;

    return E_OK;
}

/**
 * @brief Start a schedule table at an absolute start time
 */
uint8_t StartScheduleTableAbs(uint8_t table_id, TickType start) {
    if (table_id >= MAX_SCHEDULETABLES || schedule_table_list[table_id].active)
        return E_OS_LIMIT;

    ScheduleTableType *tbl = &schedule_table_list[table_id];
    tbl->start_time = start % tbl->counter->max_allowed_value;
    tbl->current_ep = 0;
    tbl->active = 1;

    return E_OK;
}

/**
 * @brief Stop a running schedule table
 */
uint8_t StopScheduleTable(uint8_t table_id) {
    if (table_id >= MAX_SCHEDULETABLES) return E_OS_LIMIT;

    schedule_table_list[table_id].active = 0;
    schedule_table_list[table_id].current_ep = 0;

    return E_OK;
}

/**
 * @brief Synchronize a running schedule table to a new offset
 */
uint8_t SyncScheduleTable(uint8_t table_id, TickType new_start_offset) {
    if (table_id >= MAX_SCHEDULETABLES) return E_OS_LIMIT;

    ScheduleTableType *tbl = &schedule_table_list[table_id];
    tbl->start_time = (tbl->counter->current_value + new_start_offset) % tbl->counter->max_allowed_value;
    tbl->current_ep = 0;
    return E_OK;
}

/**
 * @brief Tick handler for schedule tables, check and execute expiry points
 */
void ScheduleTable_Tick(CounterTypeId cid) {
    for (int i = 0; i < MAX_SCHEDULETABLES; i++) {
        ScheduleTableType *tbl = &schedule_table_list[i];

        if (!tbl->active || tbl->counter != &counter_table[cid]) continue;

    TickType rel_time = (tbl->counter->current_value + tbl->counter->max_allowed_value - tbl->start_time) % tbl->counter->max_allowed_value;

        while (tbl->current_ep < tbl->num_eps &&
               tbl->eps[tbl->current_ep].offset <= rel_time) {

            ExpiryPoint *ep = &tbl->eps[tbl->current_ep];

            switch (ep->action_type) {
                case SCH_ACTIVATETASK:
                    ActivateTask(ep->action.task_id);
                    break;
                case SCH_SETEVENT:
                    SetEvent(ep->action.set_event.task_id, ep->action.set_event.event);
                    break;
                case SCH_CALLBACK:
                    ep->action.callback_fn();
                    break;
            }

            tbl->current_ep++;
        }

        if (rel_time >= tbl->duration) {
            if (tbl->cyclic) {
                tbl->start_time = tbl->counter->current_value;
                tbl->current_ep = 0;
            } else {
                tbl->active = 0;
            }
        }
    }
}

/**
 * @brief Setup a demo schedule table with expiry points
 */
void SetupScheduleTable_Demo(void) {
    ScheduleTableType *tbl = &schedule_table_list[0];

    tbl->cyclic = 1;
    tbl->duration = 5000;
    tbl->num_eps = 3;
    tbl->counter = &counter_table[0];  // Counter 1ms
    tbl->eps[0] = (ExpiryPoint){.offset = 200, .action_type = SCH_ACTIVATETASK, .action.task_id = 1};
    tbl->eps[1] = (ExpiryPoint){.offset = 400, .action_type = SCH_CALLBACK, .action.callback_fn = my_callback1};
    tbl->eps[2] = (ExpiryPoint){.offset = 800, .action_type = SCH_CALLBACK, .action.callback_fn = my_callback};

    schedule_table_count++;
}
