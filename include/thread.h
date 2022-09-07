//
// Created by wanglu on 2022/6/28.
//

#ifndef INC_20373361_LAB4_CHALLENGE_THREAD_H
#define INC_20373361_LAB4_CHALLENGE_THREAD_H 1

#include "types.h"
#include "queue.h"
#include "trap.h"
#include "mmu.h"

#define LOG2NTCB 3
#define ENV_MAX_TCB (1 << LOG2NTCB) //一个进程最多有8个线程

#define TCBID2ENVID(tcbid) ((tcbid) >> LOG2NTCB)
#define TCBID2TCBNO(tcbid) ((tcbid) & 0x7)

//TCB status:
#define TCB_FREE 0
#define TCB_RUNNABLE 1
#define TCB_NOT_RUNNABLE 2

//thread cancel state:
#define PTHREAD_CANCEL_ENABLE 0 //default
#define PTHREAD_CANCEL_DISABLE 1

//thread cancel type:
#define PTHREAD_CANCEL_DEFERRED 0
//default   run to next cancel point
#define PTHREAD_CANCEL_ASYNCHRONOUS 1
//exit immediately

//tcb_canceled:
#define NOT_RECV_CANCEL 0
#define RECV_CANCEL 1

//tcb_exit_value:
#define THREAD_FINISH 0
#define THREAD_CANCEL 1
#define THREAD_ERROR 2

LIST_HEAD(Tcb_joined_list, TCB);

struct TCB {
    struct Trapframe tcb_tf;        // Saved registers
    u_int tcb_id;
    u_int tcb_status;
    u_int tcb_pri;
    LIST_ENTRY(TCB) tcb_sched_link;
    struct Env *tcb_env;

    u_int tcb_cancel_state;
    u_int tcb_cancel_type;
    u_int tcb_canceled;

    u_int tcb_exit_value;
    void *tcb_exit_ptr;

    LIST_ENTRY(TCB) tcb_joined_link;
    struct Tcb_joined_list tcb_joined_list;
    void **tcb_join_ptr;

    LIST_ENTRY(TCB) tcb_sem_wait_link;
};

LIST_HEAD(Tcb_list, TCB);

extern struct TCB *curtcb;	        // the current thread
extern struct Tcb_list tcb_sched_list[2]; // runnable tcb list

u_int mktcbid(u_int envid, u_int no);
int tcbid2tcb (u_int tcbid, struct TCB **ptcb);
int tcb_alloc(struct Env *e, struct TCB **new);
void tcb_free(struct TCB *t);
void tcb_destroy(struct TCB *t);
void tcb_run(struct TCB *t);

#endif //INC_20373361_LAB4_CHALLENGE_THREAD_H
