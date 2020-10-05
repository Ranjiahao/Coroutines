#include <stdio.h>
#include <stdlib.h>
#include "coroutine.h"

void* fun1(schedule_t* s, void* args) {
    printf("fun1() start\n");
    coroutine_yield(s);
    int a = *(int*)args;
    printf("fun1 exit: %d\n", a);
    return NULL;
}

void* fun2(schedule_t* s, void* args) {
    printf("fun2() start\n");
    coroutine_yield(s);
    int a = *(int*)args;
    printf("fun2 exit: %d\n", a);
    return NULL;
}

int main() {
    schedule_t *s = schedule_creat();
    int *a = (int*)malloc(sizeof(int));
    *a = 1;
    int *b = (int*)malloc(sizeof(int));
    *b = 2;
    int id1 = coroutine_creat(s, fun1, a);
    int id2 = coroutine_creat(s, fun2, b);
    coroutine_running(s, id1);
    coroutine_running(s, id2);
    while (!schedule_finish(s)) {
        coroutine_resume(s, id2);
        coroutine_resume(s, id1);
    }
    schedule_destroy(s);
}
