//
// Created by wanglu on 2022/7/3.
//

#include "lib.h"

sem_t sem;

void *consume1(void *args) {
    int a;
    int r;
    for (a = 0; a < 3; ++a) {
        r = sem_wait(&sem);
        if (r < 0) {
            user_panic("fail P at son1: %d\n",r);
        }
        writef("consumer1 get a sem, a is %d\n", a);
    }
}

void *consume2(void *args) {
    int b;
    int r;
    for (b = 0; b < 3; ++b) {
        r = sem_wait(&sem);
        if (r < 0) {
            user_panic("fail P at son2: %d\n",r);
        }
        writef("consumer2 get a sem, b is %d\n", b);
    }
}

void *consume3(void *args) {
    int c;
    int r;
    for (c = 0; c < 3; ++c) {
        r = sem_wait(&sem);
        if (r < 0) {
            user_panic("fail P at son3: %d\n",r);
        }
        writef("consumer3 get a sem, c is %d\n", c);
    }
}

void umain() {
    pthread_t thread1;
    pthread_t thread2;
    pthread_t thread3;
    sem_init(&sem,0,5);
    int r;
    pthread_create(&thread1,NULL,consume1,NULL);
    pthread_create(&thread2,NULL,consume2,NULL);
    pthread_create(&thread3,NULL,consume3,NULL);
    int value;

    int i = 0;
    for (i = 0; i < 4; ) {
        r = sem_getvalue(&sem,&value);
        if (r < 0) {
            user_panic("r is %d\n",r);
        }
        if (value == 0) {
            writef("producer  post!\n");
            sem_post(&sem);
            ++i;
        }
        syscall_yield();
    }
}
