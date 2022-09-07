//
// Created by wanglu on 2022/7/3.
//

#include "lib.h"
#include <error.h>

int a;
sem_t s;
void *test(void *ptr) {
    sem_t *semptr = (sem_t *)ptr;
    sem_wait(semptr);
    int tcbno = pthread_self() & 0x7;
    writef("thread%d gets a sem! \n", tcbno);
    int r = sem_trywait(semptr);
    if (r == -E_SEM_NO) {
        writef("no sem no wait~oh~\n");
    } else {
        writef("try wait error555555555555555\n");
    }
    sem_destroy(semptr);
}

void umain() {
    sem_t s;
    if (sem_destroy(&s) == -E_SEM_INVALID) {
        writef("sem not init, destroy fail! okkkkkkkk~\n");
    } else {
        writef("destroy error1 55555555555555\n");
    }
    sem_init(&s, 0, 0);
    int value;
    sem_getvalue(&s, &value);
    if (value == 0) {
        writef("sem init value 0 ^_^\n");
    } else {
        writef("init error 55555555555\n");
    }

    pthread_t thread;
    user_assert(pthread_create(&thread, NULL, test, &s) == 0);
    int i;
    for (i = 0; i < 10000000; ++i);
    if (sem_destroy(&s) == -E_SEM_WAIT) {
        writef("sem wait count != 0, destroy fail! okkkkkkkk~\n");
    } else {
        writef("destroy error2 55555555555555\n");
    }
    sem_post(&s);
    //while(1);
}