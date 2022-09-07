//
// Created by wanglu on 2022/7/2.
//
#include <thread.h>
#include "lib.h"
#include <error.h>

//default settings  not recv cancel
void *function0(void *ptr) {
    int i;
    int old;
    int tcbno = pthread_self() & 0x7;
    pthread_setcancelstate(0, &old);
    if (old == PTHREAD_CANCEL_ENABLE) {
        writef("son thread%d oldstate:PTHREAD_CANCEL_ENABLE\n", tcbno);
    }
    pthread_setcanceltype(0, &old);
    if (old == PTHREAD_CANCEL_DEFERRED) {
        writef("son thread%d oldstate:PTHREAD_CANCEL_DEFERRED\n", tcbno);
    }
    for (i = 0; i < 100; ++i) {
        if (i != 50) {
            writef("ha ");
        } else {
            pthread_testcancel();
        }
    }
    writef("\n");
    pthread_exit(NULL);
}
//state cancel_disable
void *function1(void *ptr) {
    int old;
    int tcbno = pthread_self() & 0x7;
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &old);
    writef("thread%d reject cancel!(PTHREAD_CANCEL_DISABLE)\n", tcbno);
    while (1);
}
//default settings run to next cancel exit
void *function2(void *ptr) {
    int tcbno = pthread_self() & 0x7;
    writef("thread%d default settings: enable-cancel and run to next cancel point!\n", tcbno);
    int i;
    for (i = 0; i < 100; ++i) {
        if (i != 50) {
            writef("ha ");
        } else {
            pthread_testcancel();
        }
    }
    writef("2testcancel error 555555555555555555555555\n");
}
//cancel exit immediately
void *function3(void *ptr) {
    int tcbno = pthread_self() & 0x7;
    int old;
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &old);
    writef("thread%d settings: enable-cancel and cancel exit immediately!\n", tcbno);
    int i;
    for (i = 0; i < 200; ++i) {
        if ((i % 2) == 0) {
            //writef("ha ");
        } else {
            pthread_testcancel();
        }
    }
    writef("3testcancel error 555555555555555555555555\n");
}

void umain() {
    pthread_t thread[5];
    char *message = "thread";
    int ret[5];
    int *join_ret;
    int i;

    ret[0] = pthread_create(&thread[0], NULL, function0, NULL);
    pthread_join(thread[0], &join_ret);
    if (*join_ret == THREAD_FINISH) {
        writef("thread%d exit finish!\n", thread[0] & 0x7);
    } else {
        writef("join error5555555555555555\n");
    }

    ret[1] = pthread_create(&thread[1], NULL, function1, NULL);

    ret[2] = pthread_create(&thread[2], NULL, function2, NULL);
    pthread_cancel(thread[2]);
    pthread_join(thread[2], &join_ret);
    if (*join_ret == THREAD_CANCEL) {
        writef("thread0 pthread_cancel thread%d okkkkkkkk~~\n", thread[2] & 0x7);
    } else {
        writef("thread0 pthread_cancel error555555555555555555555555\n");
    }

    ret[3] = pthread_create(&thread[3], NULL, function3, NULL);
    pthread_cancel(thread[3]);
    pthread_join(thread[3], &join_ret);
    if (*join_ret == THREAD_CANCEL) {
        writef("thread0 pthread_cancel thread%d okkkkkkkk~~\n", thread[3] & 0x7);
    } else {
        writef("thread0 pthread_cancel error555555555555555555555555\n");
    }

    if (pthread_cancel(thread[1]) == -E_THREAD_CANNOTCANCEL) {
        writef("thread0 pthread_cancel fail\n");
    } else {
        writef("thread0 pthread_cancel error555555555555555555555555\n");
    }

    writef("oh yeah!~\n");
}