#ifndef __COROUTINE_H__
#define __COROUTINE_H__

#include <ucontext.h>

#define CORSZ 1024
#define STACKSZ 8192

enum State {
    DEAD,
    READY,
    RUNNING,
    SUSPEND
};

struct schedule;

// 协程结构体
typedef struct coroutine {
    void* (*call_back)(struct schedule*, void*); // 回调函数指针
    void* args;          // 回调函数参数
    ucontext_t ctx;      // 协程上下文
    char stack[STACKSZ]; // 协程栈
    enum State state;    // 协程状态
} coroutine_t;

// 协程调度器
typedef struct schedule {
    coroutine_t** coroutines; // 所有协程
    int current_id;           // 当前运行的协程id
    int max_id;               // 最大下标
    ucontext_t ctx_main;      // 主流程上下文
} schedule_t;

// 创建协程调度器
schedule_t* schedule_creat();

// 创建协程，返回当前协程在调度器的下标
int coroutine_creat(schedule_t* s, void* (*call_back)(schedule_t*, void*), void* args);

// 让出CPU
void coroutine_yield(schedule_t* s);

// 恢复CPU
void coroutine_resume(schedule_t* s, int id);

// 释放调度器
void schedule_destroy(schedule_t* s);

// 判断是否所有协程都运行完了
int schedule_finish(schedule_t* s);

// 启动协程
void coroutine_running(schedule_t* s, int id);

#endif //__COROUTINE_H__
