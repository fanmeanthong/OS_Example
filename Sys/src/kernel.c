#include "kernel.h"

/* Bảng Task và biến lưu Task đang chạy */
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

/* Khởi tạo OS: đặt tất cả Task về SUSPENDED, ActivationCount = 0 */
void OS_Init(void) {
    for (int i = 0; i < TASK_NUM; i++) {
        TaskTable[i].state           = SUSPENDED;
        TaskTable[i].ActivationCount = 0;
        TaskTable[i].OsTaskActivation= 2;
    }
}

/* ActivateTask: tăng đếm, nếu vượt giới hạn trả lỗi, chuyển SUSPENDED→READY */
uint8_t ActivateTask(TaskType id) {
    TaskControlBlock *t = &TaskTable[id];

    if (t->ActivationCount >= t->OsTaskActivation) {
        return E_OS_LIMIT;
    }

    t->ActivationCount++;

    // Quan trọng: nếu task đang SUSPENDED → đưa vào READY
    if (t->state == SUSPENDED) {
        t->state = READY;
    }

    return E_OK;
}

/* TerminateTask: giảm đếm, Task RUNNING→SUSPENDED, gọi scheduler ngay */
uint8_t TerminateTask(void) {
    TaskControlBlock *t = &TaskTable[currentTask];

    if (t->ActivationCount > 0) {
        t->ActivationCount--;
    }

    // Nếu vẫn còn lượt activate → chuyển về READY
    if (t->ActivationCount > 0) {
        t->state = READY;
    } else {
        t->state = SUSPENDED;
    }

    //OS_Schedule();
    return E_OK;
}

/* ChainTask: kết hợp Terminate + Activate */
uint8_t ChainTask(TaskType id) {
    ActivateTask(id);        // Trước tiên đánh dấu task cần chạy
    TerminateTask();         // Sau đó gỡ task hiện tại
    return E_OK;
}

/* GetTaskState: lấy trạng thái hiện tại của Task */
uint8_t GetTaskState(TaskType id, TaskStateType *s) {
    *s = TaskTable[id].state;
    return E_OK;
}

/* Scheduler (co-operative round-robin):
   duyệt lần lượt TASK_NUM task, chọn READY đầu tiên */
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

void WaitEvent(EventMaskType mask) {
    if ((TaskTable[currentTask].SetEventMask & mask) == 0) {
        TaskTable[currentTask].WaitEventMask = mask;
        TaskTable[currentTask].state = WAITING;
        //OS_Schedule();
        return; // 
    }
    // Keep RUNNING
}

void SetEvent(uint8_t task_id, EventMaskType mask) {
    TaskTable[task_id].SetEventMask |= mask;

    if (TaskTable[task_id].state == WAITING &&
        (TaskTable[task_id].SetEventMask & TaskTable[task_id].WaitEventMask)) {
        
        TaskTable[task_id].state = READY;
        TaskTable[task_id].WaitEventMask = 0;
    }
}

void ClearEvent(EventMaskType mask) {
    TaskTable[currentTask].SetEventMask &= ~mask;
}

EventMaskType GetEvent(TaskType id, EventMaskType *event) {
    *event = TaskTable[id].SetEventMask;
    return E_OK;
}

uint8_t SetRelAlarm(AlarmTypeId alarm_id, TickType offset, TickType cycle) {
    if (alarm_id >= MAX_ALARMS || offset == 0) return E_OS_LIMIT;

    AlarmType *a = &alarm_table[alarm_id];
    CounterType *c = alarm_to_counter[alarm_id];

    a->state = ALARMSTATE_ACTIVE;
    a->expiry_tick = (c->current_value + offset) % c->max_allowed_value;
    a->cycle = cycle;

    return E_OK;
}

uint8_t SetAbsAlarm(AlarmTypeId alarm_id, TickType start, TickType cycle) {
    if (alarm_id >= MAX_ALARMS) return E_OS_LIMIT;

    AlarmType *a = &alarm_table[alarm_id];
    CounterType *c = alarm_to_counter[alarm_id];

    a->state = ALARMSTATE_ACTIVE;
    a->expiry_tick = start % c->max_allowed_value;
    a->cycle = cycle;

    return E_OK;
}

uint8_t CancelAlarm(AlarmTypeId alarm_id) {
    if (alarm_id >= MAX_ALARMS) return E_OS_LIMIT;

    AlarmType *a = &alarm_table[alarm_id];
    a->state = ALARMSTATE_INACTIVE;
    return E_OK;
}

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
}
void my_callback() {
    // Callback function example
    LedA_Toggle();  // Toggle LED A
}
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
    alarm_table[1].action.task_id = 1; // Kích hoạt Task Blink

    // Sau 200ms gọi Task 1, lặp lại mỗi 1000ms
    SetRelAlarm(0, 100, 250);
    SetRelAlarm(1, 200, 5000); // Kích hoạt Alarm 1 sau 200ms, lặp lại mỗi 1000ms
}

