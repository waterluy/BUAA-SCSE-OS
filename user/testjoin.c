//
// Created by wanglu on 2022/7/2.
//

#include "lib.h"

void *join_test(void *ptr) {
    char *message;
    message = (char *) ptr;
    pthread_t id = pthread_self();
    writef("thread: 0x%08x, print: %s \n", id, message);

    int i;
    for (i = 0; i < 100; ++i) {
        writef("! ");
    }
    writef("\n");
}

void umain() {
    char *message = "join test !";
    pthread_t thread1;
    int ret;

    ret = pthread_create( &thread1, NULL, join_test, (void*) message);

    int *join_ret;
    pthread_join(thread1, &join_ret);
    writef("pthread_join return : %d\n", *join_ret);

    int i;
    for (i = 0; i < 100; ++i) {
        writef("~ ");
    }
    writef("\n");
}