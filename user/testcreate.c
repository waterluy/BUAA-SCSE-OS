//
// Created by wanglu on 2022/7/2.
//

#include "lib.h"

struct arg {
    int a;
    char *s;
};

void *function(void *ptr) {
    struct arg *t = (struct arg *)ptr;
    pthread_t num = pthread_self();
    int envid = num >> 3;
    int tcbno = num & 0x7;
    writef("env%x thread%d struct arg %s%d\n", envid, tcbno, t->s, t->a + 1);
    while(1);
}
//测试create 2env 16thread. max thread print argptr
void umain() {
    int i;
    char *ccc = message;
    struct arg t[10];
    pthread_t thread[10];

int r;
r = fork();

    for (i = 0; i < 7; ++i) {
        t[i].a = i;
        t[i].s = ccc;
        pthread_create(&thread[i], NULL, function, &t[i]);
    }

    writef("threads: %x, %x, %x, %x, %x, %x, %x\n", thread[0], thread[1], thread[2], thread[3], thread[4], thread[5], thread[6]);

    if (pthread_create(&thread[i], NULL, function, &t[i]) == -E_MAX_TCB) {
        writef("one env has 8 threads at most!\n");
    }
}