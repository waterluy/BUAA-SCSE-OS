//
// Created by wanglu on 2022/6/28.
//

#include <mmu.h>
#include <error.h>
#include <env.h>
#include <kerelf.h>
#include <sched.h>
#include <pmap.h>
#include <printf.h>
#include <thread.h>

struct TCB *curtcb = NULL;            // the current tcb
struct Tcb_list tcb_sched_list[2];      // Runnable list

extern Pde *boot_pgdir;
extern char *KERNEL_SP;

u_int mktcbid(u_int envid, u_int no) { //每个进程最多有8个线程
    return (envid << LOG2NTCB) | no;
}

int tcbid2tcb (u_int tcbid, struct TCB **ptcb) {
    if (tcbid == 0) {
        *ptcb = curtcb;
        return 0;
    }

    int r;
    struct Env *e;
    u_int envid = TCBID2ENVID(tcbid);
    if ((r = envid2env(envid, &e, 0)) < 0) {
        printf("lib/thread.c: tcbid2tcb error1\n");
        return -E_BAD_ENV;
    }

    u_int tcbno = TCBID2TCBNO(tcbid);
    struct TCB *t = &(e->env_threads[tcbno]);
    if ((t->tcb_id != tcbid)/* || (t->tcb_status == TCB_FREE)*/) { //TODO check tcb status?
        *ptcb = 0;
        return -E_BAD_ENV;
    }

    *ptcb = t;
    return 0;
}
/*
 * return 0 on success
 */
int tcb_alloc(struct Env *e, struct TCB **new) {
    if (e->env_tcb_count >= ENV_MAX_TCB) {
        *new = 0;
        return -E_MAX_TCB;
    }
    struct TCB *t;
    int i;
    for (i = 0; i < ENV_MAX_TCB; ++i) {
        if (e->env_threads[i].tcb_status == TCB_FREE) {
            t = &(e->env_threads[i]);
            break;
        }
    } // 找到第一个 free 的 TCB

    t->tcb_id = mktcbid(e->env_id, i);
    t->tcb_status = TCB_RUNNABLE;
    t->tcb_env = e;

    t->tcb_cancel_state = PTHREAD_CANCEL_ENABLE;
    t->tcb_cancel_type = PTHREAD_CANCEL_DEFERRED;
    t->tcb_canceled = NOT_RECV_CANCEL;

    t->tcb_exit_value = THREAD_FINISH;
    t->tcb_exit_ptr = (void *)(&(t->tcb_exit_value));

    LIST_INIT(&(t->tcb_joined_list));

    t->tcb_tf.cp0_status = 0x1000100c;
    t->tcb_tf.regs[29] = USTACKTOP - 4 * BY2PG * (TCBID2TCBNO(t->tcb_id) + 1);

    *new = t;
    e->env_tcb_count = e->env_tcb_count + 1;
    printf("alloc a tcb for env[0x%x], tcbid: [0x%x]\n", e->env_id, t->tcb_id);
    return 0;
}

/* Overview:
 *  Free tcb t and all memory it uses.
 */
void tcb_free(struct TCB *t)
{
    struct Env *e = t->tcb_env;
    printf("env[%08x] free tcb[%08x]\n", e->env_id, t->tcb_id);
    e->env_tcb_count = e->env_tcb_count - 1;
    if (e->env_tcb_count <= 0) {
        env_free(e);
    }
    t->tcb_status = TCB_FREE;
    LIST_REMOVE(t, tcb_sched_list);
}

/* Overview:
 *  Free TCB t, and schedule to run a new t if t is the current tcb.
 */
void tcb_destroy(struct TCB *t)
{
    /* Hint: free t. */
    tcb_free(t);

    /* Hint: schedule to run a new thread. */
    if (curtcb == t) {
        curtcb = NULL;
        /* Hint: Why this? */
        bcopy((void *)KERNEL_SP - sizeof(struct Trapframe),
              (void *)TIMESTACK - sizeof(struct Trapframe),
              sizeof(struct Trapframe));
        printf("kill a thread ... \n");
        sched_yield();
    }
}

extern struct Env *curenv;
extern void env_pop_tf(struct Trapframe *tf, int id);
extern void lcontext(u_int contxt);

void tcb_run(struct TCB *t)
{
    /* Step 1: save register state of curenv. */
    /* Hint: if there is an environment running,
     *   you should switch the context and save the registers.
     *   You can imitate env_destroy() 's behaviors.*/
    if (curtcb) {
        struct Trapframe *old;
        old = (struct Trapframe *)(TIMESTACK - sizeof(struct Trapframe));
        bcopy((void *)old, &(curtcb->tcb_tf), sizeof(struct Trapframe));
        curtcb->tcb_tf.pc = curtcb->tcb_tf.cp0_epc;
    }

    /* Step 2: Set 'curenv' to the new environment. */
    curtcb = t;
    curenv = t->tcb_env;

    /* Step 3: Use lcontext() to switch to its address space. */
    lcontext((int)(curenv->env_pgdir));

    /* Step 4: Use env_pop_tf() to restore the environment's
     *   environment   registers and return to user mode.
     *
     * Hint: You should use GET_ENV_ASID there. Think why?
     *   (read <see mips run linux>, page 135-144)
     */
    env_pop_tf(&(curtcb->tcb_tf), GET_ENV_ASID(curenv->env_id));
}
