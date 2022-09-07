//
// Created by wanglu on 2022/7/3.
//

#include "lib.h"

sem_t sem;
int res;

void *printhhh(void *args) {
    int a;
    int r;
    for (a = 0; a < 10; ++a) {
        r = sem_wait(&sem);
        res = res + 1;
        writef("consumer1 get a sem, now res is %d\n",res);
        r = sem_post(&sem);
    }
}

/*
void *printhhh(void *args) {
    int a;
    int id = pthread_self() & 0x7;
    int data;

    for (a = 0; a < 10; ++a) {
        //r = sem_wait(&sem);
        writef("thread%d steps into critical area, now a is %d\n", id, a);
        //r = sem_post(&sem);
    }
}*/

void umain() {
    pthread_t thread1;
    pthread_t thread2;
    pthread_t thread3;
    sem_init(&sem,0,1);
    int value;
    sem_getvalue(&sem,&value);
    if (value == 1) {
        writef("sem value is 1!\n");
    }

    int r;
    pthread_create(&thread1,NULL,printhhh,NULL);
    pthread_create(&thread2,NULL,printhhh,NULL);
    pthread_create(&thread3,NULL,printhhh,NULL);

    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    pthread_join(thread3, NULL);

    if (res == 30) {
        writef("res is 30! oh yeah!~\n");
    } else {
        writef("error------\n");
    }
}
