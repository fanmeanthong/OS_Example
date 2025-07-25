#include "kernel.h"

/* Bảng Task và biến lưu Task đang chạy */
TaskControlBlock TaskTable[TASK_NUM];
TaskType    currentTask = 0;
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