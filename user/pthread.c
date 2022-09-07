//
// Created by wanglu on 2022/6/28.
//

#include "lib.h"
#include <error.h>
#include <mmu.h>
#include <thread.h>

int pthread_create(pthread_t * thread,
                  const pthread_attr_t * attr,
                  void * (*start_routine)(void *),
                  void *arg) {
    int tcbno, r;
    if ((tcbno = syscall_thread_alloc()) < 0) {
        return tcbno;
    }
    if (start_routine == NULL) {
        return -E_INVAL;
    }
    struct TCB *t = &(env->env_threads[tcbno]);
    t->tcb_tf.pc = start_routine;
    t->tcb_tf.regs[4] = arg; //arguments struct
    t->tcb_tf.regs[31] = exit; //jr ra after start_routine     ra -> exit

    if ((r = syscall_set_thread_status(t->tcb_id, TCB_RUNNABLE)) < 0) {
        return r;
    }
    *thread = t->tcb_id;
    return 0;
}

void pthread_exit(void *retval) {
    struct TCB *t;
    u_int tcbid = syscall_get_thread_id(&t);
    t->tcb_exit_value = THREAD_FINISH;
    if (retval != NULL) {
        t->tcb_exit_ptr = retval;
    }
    syscall_thread_destroy(tcbid);
}

int pthread_setcancelstate(int state,int *oldstate) {
    struct TCB *t;
    u_int tcbid = syscall_get_thread_id(&t);
    if ((state != PTHREAD_CANCEL_ENABLE) && (state != PTHREAD_CANCEL_DISABLE)) {
        return -E_INVAL;
    }
    if (oldstate != NULL) {
        *oldstate = t->tcb_cancel_state;
    }
    t->tcb_cancel_state = state;
    return 0;
}

int pthread_setcanceltype(int type,int *oldtype) {
    struct TCB *t;
    u_int tcbid = syscall_get_thread_id(&t);
    if ((type != PTHREAD_CANCEL_DEFERRED) && (type != PTHREAD_CANCEL_ASYNCHRONOUS)) {
        return -E_INVAL;
    }
    if (oldtype != NULL) {
        *oldtype = t->tcb_cancel_type;
    }
    t->tcb_cancel_type = type;
    return 0;
}

void pthread_testcancel() {
    struct TCB *t;
    u_int tcbid = syscall_get_thread_id(&t);
    if ((t->tcb_canceled == RECV_CANCEL) &&
            (t->tcb_cancel_state == PTHREAD_CANCEL_ENABLE) &&
            (t->tcb_cancel_type == PTHREAD_CANCEL_DEFERRED)) {
        t->tcb_exit_value = THREAD_CANCEL;
        writef("test cancek ok!\n");
        syscall_thread_destroy(tcbid);
    }
}

int pthread_cancel(pthread_t thread) {
    struct TCB *t = &(env->env_threads[thread & 0x7]);
    if ((t->tcb_id != thread) || (t->tcb_status == TCB_FREE)) {
        return -E_THREAD_NOTFOUND;
    }
    if (t->tcb_cancel_state == PTHREAD_CANCEL_DISABLE) {
        return -E_THREAD_CANNOTCANCEL;
    }

    if (t->tcb_cancel_type == PTHREAD_CANCEL_ASYNCHRONOUS) {
        t->tcb_exit_value = THREAD_CANCEL;
        syscall_thread_destroy(thread);
    } else {
        t->tcb_canceled = RECV_CANCEL;
    }
    return 0;
}

int pthread_join(pthread_t thread, void **retval) {
    return syscall_thread_join(thread, retval);
}

pthread_t pthread_self(void) {
    return syscall_get_thread_id(NULL);
}