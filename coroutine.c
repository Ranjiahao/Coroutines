#include <stdlib.h>
#include "coroutine.h"

// 创建协程调度器
schedule_t* schedule_creat() {
    schedule_t* s = (schedule_t*)malloc(sizeof(schedule_t));
    if (s) {
        s->coroutines = (coroutine_t**)calloc(CORSZ, sizeof(coroutine_t*));
        s->current_id = -1;
        s->max_id = -1;
    }
    return s;
}

// 协程执行函数
static void* main_fun(schedule_t* s) {
    int id = s->current_id;
    if (id != -1) {
        coroutine_t* c = s->coroutines[id];
        c->call_back(s, c->args);
        // 目标函数执行完毕，进入DEAD状态
        c->state = DEAD;
        // -1表示没有正在运行的协程
        s->current_id = -1;
    }
    return NULL;
}

// 创建协程，返回当前协程在调度器的下标
int coroutine_creat(schedule_t* s, void* (*call_back)(schedule_t*, void*), void* args) {
    coroutine_t* c = NULL;
    int i;
    for (i = 0; i <= s->max_id; ++i) {
        c = s->coroutines[i];
        if (c->state == DEAD) {
            break;
        }
    }
    if (i > s->max_id) {
        s->coroutines[i] = (coroutine_t*)malloc(sizeof(coroutine_t));
        s->max_id++;
    }
    c = s->coroutines[i];
    c->call_back = call_back;
    c->args = args;
    c->state = READY;
    getcontext(&c->ctx);
    c->ctx.uc_stack.ss_sp = c->stack;
    c->ctx.uc_stack.ss_size = STACKSZ;
    c->ctx.uc_stack.ss_flags = 0;
    c->ctx.uc_link = &s->ctx_main;
    makecontext(&c->ctx, (void (*)())main_fun, 1, s);
    return i;
}

// 获取协程状态
static enum State get_status(schedule_t* s, int id) {
    coroutine_t* c = s->coroutines[id];
    if (c == NULL) {
        return DEAD;
    }
    return c->state;
}

// 启动协程
void coroutine_running(schedule_t* s, int id) {
    int st = get_status(s, id);
    if (st != DEAD) {
        coroutine_t* c = s->coroutines[id];
        c->state = RUNNING;
        s->current_id = id;
        swapcontext(&s->ctx_main, &c->ctx);
    }
}

// 让出CPU
void coroutine_yield(schedule_t* s) {
    int id = s->current_id;
    if (id != -1) {
        coroutine_t* c = s->coroutines[id];
        c->state = SUSPEND;
        s->current_id = -1;
        swapcontext(&c->ctx, &s->ctx_main);
    }
}

// 恢复CPU
void coroutine_resume(schedule_t* s, int id) {
    coroutine_t* c = s->coroutines[id];
    if (c != NULL && c->state == SUSPEND) {
        c->state = RUNNING;
        s->current_id = id;
        swapcontext(&s->ctx_main, &c->ctx);
    }
}

// 删除协程
static void delete_coroutine(schedule_t* s, int id) {
    coroutine_t* c = s->coroutines[id];
    if (c != NULL) {
        free(c);
        s->coroutines[id] = NULL;
    }
}

// 释放调度器
void schedule_destroy(schedule_t* s) {
    int i;
    for (i = 0; i <= s->max_id; ++i) {
        delete_coroutine(s, i);
    }
    free(s->coroutines);
    free(s);
}

// 判断所有协程都运行完了
int schedule_finish(schedule_t* s) {
    if (s->current_id != -1) {
        return 0;
    }
    int i;
    for (i = 0; i <= s->max_id; ++i) {
        if (s->coroutines[i]->state != DEAD) {
            return 0;
        }
    }
    return 1;
}
