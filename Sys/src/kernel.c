#include "kernel.h"
#include "setup.h"
#include <string.h>
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
    //OS_RequestSchedule(); 
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
        OS_RequestSchedule();           
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
    if (alarm_id >= MAX_ALARMS) return E_OS_ID;
    if (offset == 0)            return E_OS_VALUE;

    AlarmType *a = &alarm_table[alarm_id];
    CounterType *c = alarm_to_counter[alarm_id];
    if (!c) return E_OS_STATE;

    if (cycle && cycle < c->min_cycle) return E_OS_VALUE;

    a->state = ALARMSTATE_ACTIVE;
    a->expiry_tick = (c->current_value + offset) % c->max_allowed_value;
    a->cycle = cycle;
    return E_OK;
}

uint8_t SetAbsAlarm(AlarmTypeId alarm_id, TickType start, TickType cycle) {
    if (alarm_id >= MAX_ALARMS) return E_OS_ID;

    AlarmType *a = &alarm_table[alarm_id];
    CounterType *c = alarm_to_counter[alarm_id];
    if (!c) return E_OS_STATE;

    if (cycle && cycle < c->min_cycle) return E_OS_VALUE;

    a->state = ALARMSTATE_ACTIVE;
    a->expiry_tick = start % c->max_allowed_value;
    a->cycle = cycle;
    return E_OK;
}

uint8_t CancelAlarm(AlarmTypeId alarm_id) {
    if (alarm_id >= MAX_ALARMS) return E_OS_ID;

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
uint8_t StartScheduleTableRel(uint8_t id, TickType offset) {
    if (id >= MAX_SCHEDULETABLES)            return E_OS_ID;
    ScheduleTableType *t = &schedule_table_list[id];
    if (t->state != ST_STOPPED)              return E_OS_STATE;
    if (offset >= t->counter->max_allowed_value) return E_OS_VALUE;

    TickType cur = t->counter->current_value, max = t->counter->max_allowed_value;
    t->start_time = (cur + offset) % max;
    t->current_ep = 0;
    t->state      = ST_WAITING_START;
    return E_OK;
}

/**
 * @brief Start a schedule table at an absolute start time
 */
uint8_t StartScheduleTableAbs(uint8_t id, TickType start) {
    if (id >= MAX_SCHEDULETABLES)            return E_OS_ID;
    ScheduleTableType *t = &schedule_table_list[id];
    if (t->state != ST_STOPPED)              return E_OS_STATE;

    TickType max = t->counter->max_allowed_value;
    t->start_time = start % max;
    t->current_ep = 0;
    t->state      = ST_WAITING_START;
    return E_OK;
}

/**
 * @brief Stop a running schedule table
 */
uint8_t StopScheduleTable(uint8_t id) {
    if (id >= MAX_SCHEDULETABLES)            return E_OS_ID;
    ScheduleTableType *t = &schedule_table_list[id];
    if (t->state == ST_STOPPED)              return E_OS_NOFUNC;

    t->state = ST_STOPPED;
    t->current_ep = 0;
    return E_OK;
}

/**
 * @brief Synchronize a running schedule table to a new offset
 */
uint8_t SyncScheduleTable(uint8_t id, TickType new_offset) {
    if (id >= MAX_SCHEDULETABLES)            return E_OS_ID;
    ScheduleTableType *t = &schedule_table_list[id];
    if (t->state == ST_STOPPED)              return E_OS_STATE;

    TickType cur = t->counter->current_value, max = t->counter->max_allowed_value;
    t->start_time = (cur + new_offset) % max;
    t->current_ep = 0;
    t->state      = ST_WAITING_START;
    return E_OK;
}

/**
 * @brief Calculate difference with wrap-around for tick counters
 */
static inline TickType diff_wrap(TickType cur, TickType start, TickType max) {
    return (cur >= start) ? (cur - start) : (max - start + cur);
}

/**
 * @brief Tick handler for schedule tables, check and execute expiry points
 */
void ScheduleTable_Tick(CounterTypeId cid) {
    CounterType *c = &counter_table[cid];

    for (int i = 0; i < MAX_SCHEDULETABLES; i++) {
        ScheduleTableType *t = &schedule_table_list[i];
        if (t->counter != c || t->state == ST_STOPPED) continue;

        TickType cur = c->current_value;
        TickType max = c->max_allowed_value;

        // Khoảng thời gian kể từ start_time (xử lý wrap)
        TickType elapsed_from_start = diff_wrap(cur, t->start_time, max);

        // 1) Đang chờ start, có thể đã trễ tick start_time
        if (t->state == ST_WAITING_START) {
            if (elapsed_from_start < t->duration) {
                // Đã tới (hoặc qua) mốc start trong chu kỳ hiện tại
                t->state = ST_RUNNING;
                t->current_ep = 0;

                // Catch-up: chạy mọi EP có offset <= elapsed_from_start
                while (t->current_ep < t->num_eps &&
                       t->eps[t->current_ep].offset <= elapsed_from_start) {
                    ExpiryPoint *ep = &t->eps[t->current_ep];
                    if (ep->action_type == SCH_ACTIVATETASK)      ActivateTask(ep->action.task_id);
                    else if (ep->action_type == SCH_SETEVENT)     SetEvent(ep->action.set_event.task_id, ep->action.set_event.event);
                    else                                          ep->action.callback_fn();
                    t->current_ep++;
                }
            } else {
                // Bị lỡ cả 1 (hoặc nhiều) chu kỳ mà chưa start
                if (t->cyclic) {
                    TickType periods_skipped = elapsed_from_start / t->duration;
                    t->start_time = (t->start_time + periods_skipped * t->duration) % max;
                    // vẫn WAITING_START
                } else {
                    t->state = ST_STOPPED;
                    t->current_ep = 0;
                }
            }
            continue;
        }

        // 2) RUNNING: xử lý EP và kết thúc chu kỳ nếu tới hạn
        if (t->state == ST_RUNNING) {
            // Catch-up các EP có offset <= elapsed
            while (t->current_ep < t->num_eps &&
                   t->eps[t->current_ep].offset <= elapsed_from_start) {
                ExpiryPoint *ep = &t->eps[t->current_ep];
                if (ep->action_type == SCH_ACTIVATETASK)      ActivateTask(ep->action.task_id);
                else if (ep->action_type == SCH_SETEVENT)     SetEvent(ep->action.set_event.task_id, ep->action.set_event.event);
                else                                          ep->action.callback_fn();
                t->current_ep++;
            }

            // Hết chu kỳ (hoặc vượt vì trễ) → chuẩn bị chu kỳ mới
            if (elapsed_from_start >= t->duration) {
                if (t->cyclic) {
                    TickType periods = elapsed_from_start / t->duration;
                    t->start_time = (t->start_time + periods * t->duration) % max;
                    t->current_ep = 0;
                    t->state      = ST_WAITING_START;

                    // Trường hợp cùng tick → start luôn & catch-up
                    TickType e2 = diff_wrap(cur, t->start_time, max);
                    if (e2 < t->duration) {
                        t->state = ST_RUNNING;
                        while (t->current_ep < t->num_eps &&
                               t->eps[t->current_ep].offset <= e2) {
                            ExpiryPoint *ep = &t->eps[t->current_ep];
                            if (ep->action_type == SCH_ACTIVATETASK)      ActivateTask(ep->action.task_id);
                            else if (ep->action_type == SCH_SETEVENT)     SetEvent(ep->action.set_event.task_id, ep->action.set_event.event);
                            else                                          ep->action.callback_fn();
                            t->current_ep++;
                        }
                    }
                } else {
                    t->state = ST_STOPPED;
                    t->current_ep = 0;
                }
            }
        }
    }
}

// =====================
// Schedule Table Setup APIs
// =====================
/**
 * @brief Setup demo schedule table with expiry points
 */
void SetupScheduleTable_Demo(void) {
    ScheduleTableType *t = &schedule_table_list[0];
    t->counter  = &counter_table[0];
    t->duration = 2000;
    t->cyclic   = 1; 
    t->num_eps  = 3;
    t->eps[0]   = (ExpiryPoint){ .offset=200, .action_type=SCH_ACTIVATETASK, .action.task_id=1 };
    t->eps[1]   = (ExpiryPoint){ .offset=400, .action_type=SCH_CALLBACK,     .action.callback_fn=my_callback1 };
    t->eps[2]   = (ExpiryPoint){ .offset=800, .action_type=SCH_CALLBACK,     .action.callback_fn=my_callback };
    schedule_table_count++;
}

/**
 * @brief Setup alarm to activate LED tick task every 50ms
 */
void SetupAlarm_LedTick(void) {
    AlarmTypeId alarm_id = 2;
    CounterTypeId counter_id = 0;

    alarm_to_counter[alarm_id] = &counter_table[counter_id];
    counter_table[counter_id].alarm_list[counter_table[counter_id].num_alarms++] = &alarm_table[alarm_id];

    alarm_table[alarm_id].action_type = ALARMACTION_ACTIVATETASK;
    alarm_table[alarm_id].action.task_id = TASK_LED_TICK_ID;

    SetRelAlarm(alarm_id, 50, 50); // 50ms offset, 50ms cycle
}

/**
 * @brief Setup schedule table to change LED mode periodically
 */
void SetupScheduleTable_Mode(void) {
    ScheduleTableType *t = &schedule_table_list[1]; // dùng bảng số 1
    t->counter  = &counter_table[0]; // counter 1ms
    t->duration = 5000;              // 5 giây
    t->cyclic   = 1;
    t->num_eps  = 3;

    // Dùng callback để đổi mode (đơn giản & rõ ràng)
    t->eps[0] = (ExpiryPoint){ .offset = 0,    .action_type = SCH_CALLBACK, .action.callback_fn = SetMode_Normal  };
    t->eps[1] = (ExpiryPoint){ .offset = 2000, .action_type = SCH_CALLBACK, .action.callback_fn = SetMode_Warning };
    t->eps[2] = (ExpiryPoint){ .offset = 3000, .action_type = SCH_CALLBACK, .action.callback_fn = SetMode_Off     };

    schedule_table_count++;
}

// =====================
// Resource Management APIs
// =====================
/**
 * @brief Get a resource (priority ceiling protocol)
 * @param ResID Resource ID
 * @return Error code
 *
uint8_t GetResource(ResourceType ResID)
{
    if (ResID >= MAX_RESOURCES) return E_OS_ID;
    ResourceControlBlock *r = &ResourceTable[ResID];
    TaskControlBlock *t = &TaskTable[currentTask];

    if (r->owner != INVALID_TASK) {
        return E_OS_STATE; // đã có task khác giữ
    }

    r->owner = currentTask;
    t->resTaken = ResID;

    // PCP: nâng prio task lên ceiling nếu cần
    if (t->curPrio < r->ceilingPrio) {
        t->curPrio = r->ceilingPrio;
    }
    return E_OK;
}*/

/**
 * @brief Release a resource (restore priority)
 * @param ResID Resource ID
 * @return Error code
 *
uint8_t ReleaseResource(ResourceType ResID)
{
    if (ResID >= MAX_RESOURCES) return E_OS_ID;
    ResourceControlBlock *r = &ResourceTable[ResID];
    TaskControlBlock *t = &TaskTable[currentTask];

    if (r->owner != currentTask) {
        return E_OS_STATE; // không phải chủ sở hữu
    }

    r->owner = INVALID_TASK;
    t->resTaken = INVALID_TASK;

    // phục hồi priority gốc
    t->curPrio = t->basePrio;
    return E_OK;
}*/

// =====================
// IOC Channel Table
// =====================
IocChannelType IocChannelTable[MAX_IOC_CHANNELS];

// =====================
// IOC APIs
// =====================

/**
 * @brief Initialize an IOC channel
 * @param ch Channel index
 * @param data_size Size of each data element
 * @param receivers Array of receiver task IDs
 * @param num Number of receivers
 */
void Ioc_InitChannel(uint8_t ch, uint8_t data_size, TaskType *receivers, uint8_t num) {
    IocChannelTable[ch].used = 1;
    IocChannelTable[ch].data_size = data_size;
    IocChannelTable[ch].num_receivers = num;
    for (int i = 0; i < num; i++) {
        IocChannelTable[ch].receivers[i] = receivers[i];
    }
    IocChannelTable[ch].head = 0;
    IocChannelTable[ch].tail = 0;
    IocChannelTable[ch].count = 0;
    IocChannelTable[ch].flag_new = 0;
}

/**
 * @brief Send data to IOC channel (overwrite if full)
 * @param ch Channel index
 * @param data Pointer to data
 * @return Error code
 */
uint8_t IocSend(uint8_t ch, void *data) {
    if (ch >= MAX_IOC_CHANNELS || !IocChannelTable[ch].used) return E_OS_ID;
    IocChannelType *c = &IocChannelTable[ch];

    memcpy(c->buffer[c->head], data, c->data_size);
    c->head = (c->head + 1) % IOC_BUFFER_SIZE;
    if (c->count < IOC_BUFFER_SIZE) c->count++;
    else c->tail = (c->tail + 1) % IOC_BUFFER_SIZE;

    c->flag_new = 1;

    // Notify receivers via event
    for (int i = 0; i < c->num_receivers; i++) {
        SetEvent(c->receivers[i], (1U << ch));
    }
    return E_OK;
}

/**
 * @brief Receive one data element from IOC channel
 * @param ch Channel index
 * @param data Pointer to buffer for received data
 * @return Error code
 */
uint8_t IocReceive(uint8_t ch, void *data) {
    if (ch >= MAX_IOC_CHANNELS || !IocChannelTable[ch].used) return E_OS_ID;
    IocChannelType *c = &IocChannelTable[ch];

    if (c->count == 0) return E_OS_NOFUNC;

    memcpy(data, c->buffer[c->tail], c->data_size);
    c->tail = (c->tail + 1) % IOC_BUFFER_SIZE;
    c->count--;
    c->flag_new = 0;

    return E_OK;
}

/**
 * @brief Receive multiple data elements from IOC channel
 * @param ch Channel index
 * @param data Pointer to buffer for received data
 * @param num Number of elements to receive
 * @return Error code
 */
uint8_t IocReceiveGroup(uint8_t ch, void *data, uint8_t num) {
    if (ch >= MAX_IOC_CHANNELS || !IocChannelTable[ch].used) return E_OS_ID;
    IocChannelType *c = &IocChannelTable[ch];
    if (c->count < num) return E_OS_NOFUNC;

    for (int i = 0; i < num; i++) {
        memcpy((uint8_t*)data + i * c->data_size, c->buffer[c->tail], c->data_size);
        c->tail = (c->tail + 1) % IOC_BUFFER_SIZE;
        c->count--;
    }
    if (c->count == 0) c->flag_new = 0;
    return E_OK;
}

/**
 * @brief Check if IOC channel has new data
 * @param ch Channel index
 * @return 1 if new data, 0 otherwise
 */
uint8_t IocHasNewData(uint8_t ch) {
    if (ch >= MAX_IOC_CHANNELS || !IocChannelTable[ch].used) return 0;
    return IocChannelTable[ch].flag_new;
}

void OS_RequestSchedule(void) {
    SCB_ICSR = ICSR_PENDSVSET;
}