#ifndef __THREAD_THREAD_H
#define __THREAD_THREAD_H

#include "../header/stdint.h"

typedef void thread_func(void *);

enum task_status
{
    TASK_RUNNING,
    TASK_READY,
    TASK_BLOCKED,
    TASK_WAITING,
    TASK_HANGING,
    TASK_DIED
};
#endif