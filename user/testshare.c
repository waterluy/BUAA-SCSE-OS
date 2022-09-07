//
// Created by wanglu on 2022/7/3.
//

#include "lib.h"

int a;

void *share(void *args) {
    int *x = (int *)args;
    writef("arg is %d, a is %d\n", *x, a);
    *x = 20373361;
    a = 6;
    pthread_exit(NULL);
}

void umain() {
    pthread_t p;
    int x = 19521025;
    user_assert(pthread_create(&p, NULL, share, (void *)&x) == 0);
    user_assert(pthread_join(p, NULL) == 0);
    writef("x is now %d, a is now %d\n", x, a);
}