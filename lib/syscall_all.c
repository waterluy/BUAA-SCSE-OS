#include "../drivers/gxconsole/dev_cons.h"
#include <mmu.h>
#include <env.h>
#include <printf.h>
#include <pmap.h>
#include <sched.h>
#include <thread.h>
#include <semaphore.h>
#include <error.h>

extern char *KERNEL_SP;
extern struct Env *curenv;
extern struct TCB *curtcb;

int sys_get_thread_id(int sysno, struct TCB **nowtcb, int a2, int a3, int a4, int a5) {
    int r;
    if (nowtcb != NULL) {
        //if ((r = tcbid2tcb(curtcb->tcb_id, nowtcb)) < 0) {
         //   return r;
        //}
        *nowtcb = curtcb;
    }
    return curtcb->tcb_id;
}

int sys_thread_alloc(int sysno, int a1, int a2, int a3, int a4, int a5) {
    if (curenv == NULL) {
        return -1;
    }

    int r;
    struct TCB *t;
    if ((r = tcb_alloc(curenv, &t)) < 0) {
        return r;
    }
    //bcopy((void *)KERNEL_SP - sizeof(struct Trapframe), (void *)&(t->tcb_tf), sizeof(struct Trapframe));
    t->tcb_pri = curenv->env_pri;
    t->tcb_status = TCB_NOT_RUNNABLE;
    //t->tcb_tf.regs[2] = 0;
    //t->tcb_tf.pc = t->tcb_tf.cp0_epc;
    return (TCBID2TCBNO(t->tcb_id));
}

int sys_set_thread_status(int sysno, u_int tcbid, u_int status, int a3, int a4, int a5) {
    struct TCB *t;
    int r;
    if ((status != TCB_RUNNABLE) && (status != TCB_NOT_RUNNABLE) && (status != TCB_FREE)) {
        return -E_INVAL;
    }

    if ((r = tcbid2tcb(tcbid, &t)) < 0) {
        return r;
    }

    if ((t->tcb_status != TCB_RUNNABLE) && (status == ENV_RUNNABLE)) {
        LIST_INSERT_HEAD(&tcb_sched_list[0], t, tcb_sched_link);
    } else if ((t->tcb_status == TCB_RUNNABLE) && (status != TCB_RUNNABLE)) {
        LIST_REMOVE(t, tcb_sched_link);
    }
    t->tcb_status = status;
    return 0;
}

int sys_thread_destroy(int sysno, u_int tcbid, int a2, int a3, int a4, int a5) {
    int r;
    struct TCB *t;
    if ((r = tcbid2tcb(tcbid, &t)) < 0) {
        return r;
    }
    if (t->tcb_status == TCB_FREE) {
        return -E_INVAL;
    }

    struct TCB *wait;
    while (!LIST_EMPTY(&(t->tcb_joined_list))) {
        wait = LIST_FIRST(&(t->tcb_joined_list));
        LIST_REMOVE(wait, tcb_joined_link);
        if (wait->tcb_join_ptr != NULL) {
            *(wait->tcb_join_ptr) = t->tcb_exit_ptr;
        }
        if ((r = sys_set_thread_status(0, wait->tcb_id,
                      TCB_RUNNABLE, 0 ,0, 0)) < 0) {
            return r;
        }
    }

    printf("env[0x%08x] destroying tcb[0x%08x]\n", curenv->env_id, t->tcb_id);
    tcb_destroy(t);
    return 0;
}

int sys_thread_join(int sysno, u_int tcbid, void **retval, int a3, int a4, int a5) {
    int r;
    struct TCB *t;
    if ((r = tcbid2tcb(tcbid, &t)) < 0) {
        return r;
    }

    if (t->tcb_status == TCB_FREE) {
        if  (retval != NULL) {
            *retval = t->tcb_exit_ptr;
        }
        return 0;
    }

    LIST_INSERT_HEAD(&(t->tcb_joined_list),
                     curtcb, tcb_joined_link);
    curtcb->tcb_join_ptr = retval;
    if ((r = sys_set_thread_status(0, curtcb->tcb_id,
                  TCB_NOT_RUNNABLE, 0 ,0, 0)) < 0) {
        return r;
    }
    sys_yield();

    struct Trapframe *trap = (struct Trapframe *)(KERNEL_SP - sizeof(struct Trapframe));
    trap->regs[2] = 0;
    sys_yield();
    return 0;
}

int sys_sem_init (int sysno, sem_t *sem, int pshared, unsigned int value, int a4, int a5) {
    if (sem == NULL) {
        return -E_SEM_INVALID;
    }
    sem->sem_value = value;
    sem->sem_shared = pshared;
    sem->sem_wait_count = 0;
    sem->sem_status = SEM_VALID;
    LIST_INIT(&(sem->sem_wait_list));
    sem->sem_envid = curenv->env_id;
    return 0;
}

int sys_sem_destroy(int sysno, sem_t *sem, int a2, int a3, int a4, int a5) {
    if ((sem == NULL) || (sem->sem_status != SEM_VALID)) {
        return -E_SEM_INVALID;
    }
    if (sem->sem_wait_count != 0) {
        return -E_SEM_WAIT;
    }
    if ((sem->sem_sem_shared == 0) && (sem->sem_envid != curenv->env_id)) {
        return -E_SEM_WRONG_ENV;
    }
    sem->sem_status = SEM_INVALID;
    printf("destroy a sem!\n");
    return 0;
}

int sys_sem_post(int sysno, sem_t *sem, int a2, int a3, int a4, int a5) {
    if ((sem == NULL) || (sem->sem_status != SEM_VALID)) {
        return -E_SEM_INVALID;
    }
    if ((sem->sem_sem_shared == 0) && (sem->sem_envid != curenv->env_id)) {
        return -E_SEM_WRONG_ENV;
    }

    int r;
    if ((sem->sem_value > 0) || (sem->sem_wait_count == 0)) {
        sem->sem_value = sem->sem_value + 1;
        printf("thread[0x%x] post a sem value now value is %d\n", curtcb->tcb_id, sem->sem_value + 1);
    } else {
        sem->sem_wait_count = sem->sem_wait_count - 1;
        struct TCB *t = LIST_FIRST(&(sem->sem_wait_list));
        LIST_REMOVE(t, tcb_sem_wait_link);
        if ((r = sys_set_thread_status(0, t->tcb_id,
                          TCB_RUNNABLE, 0, 0, 0)) < 0) {
            return r;
        }
        printf("thread[0x%x] posts,thread[0x%x] gets, now value is %d\n", curtcb->tcb_id, t->tcb_id, sem->sem_value);
    }

    return 0;
}

int sys_sem_wait(int sysno, sem_t *sem, int a2, int a3, int a4, int a5) {
    if ((sem == NULL) || (sem->sem_status != SEM_VALID)) {
        return -E_SEM_INVALID;
    }
    if ((sem->sem_sem_shared == 0) && (sem->sem_envid != curenv->env_id)) {
        return -E_SEM_WRONG_ENV;
    }

    if (sem->sem_value > 0) {
        sem->sem_value = sem->sem_value - 1;
        printf("tcb[0x%x] gets a sem value, no wait!\n",curtcb->tcb_id);
        return 0;
    }
    int r;
    sem->sem_wait_count = sem->sem_wait_count + 1;
    LIST_INSERT_TAIL(&(sem->sem_wait_list), curtcb, tcb_sem_wait_link);
    if ((r = sys_set_thread_status(0, curtcb->tcb_id, TCB_NOT_RUNNABLE,
                                   0, 0, 0)) < 0) {
        return r;
    }
    struct Trapframe *trap = (struct Trapframe *)(KERNEL_SP - sizeof(struct Trapframe));
    trap->regs[2] = 0;
    printf("tcb[0x%x] sem wait yield.\n",curtcb->tcb_id);
    sys_yield();
    return 0;
}

int sys_sem_try_wait(int sysno, sem_t *sem, int a2, int a3, int a4, int a5) {
    if ((sem == NULL) || (sem->sem_status != SEM_VALID)) {
        return -E_SEM_INVALID;
    }
    if ((sem->sem_sem_shared == 0) && (sem->sem_envid != curenv->env_id)) {
        return -E_SEM_WRONG_ENV;
    }
    if (sem->sem_value > 0) {
        sem->sem_value = sem->sem_value - 1;
        return 0;
    }
    return -E_SEM_NO;
}

int sys_sem_get_value(int sysno, sem_t * sem, int * sval, int a3, int a4, int a5) {
    if ((sem == NULL) || (sem->sem_status != SEM_VALID)) {
        return -E_SEM_INVALID;
    }
    if ((sem->sem_sem_shared == 0) && (sem->sem_envid != curenv->env_id)) {
        return -E_SEM_WRONG_ENV;
    }
    if (sval != NULL) {
        *sval = sem->sem_value;
    }
    return 0;
}

/* Overview:
 * 	This function is used to print a character on screen.
 *
 * Pre-Condition:
 * 	`c` is the character you want to print.
 */
void sys_putchar(int sysno, int c, int a2, int a3, int a4, int a5)
{
	printcharc((char) c);
	return ;
}

/* Overview:
 * 	This function enables you to copy content of `srcaddr` to `destaddr`.
 *
 * Pre-Condition:
 * 	`destaddr` and `srcaddr` can't be NULL. Also, the `srcaddr` area
 * 	shouldn't overlap the `destaddr`, otherwise the behavior of this
 * 	function is undefined.
 *
 * Post-Condition:
 * 	the content of `destaddr` area(from `destaddr` to `destaddr`+`len`) will
 * be same as that of `srcaddr` area.
 */
void *memcpy(void *destaddr, void const *srcaddr, u_int len)
{
	char *dest = destaddr;
	char const *src = srcaddr;

	while (len-- > 0) {
		*dest++ = *src++;
	}

	return destaddr;
}

/* Overview:
 *	This function provides the environment id of current process.
 *
 * Post-Condition:
 * 	return the current environment id
 */
u_int sys_getenvid(void)
{
	return curenv->env_id;
}

/* Overview:
 *	This function enables the current process to give up CPU.
 *
 * Post-Condition:
 * 	Deschedule current process. This function will never return.
 *
 * Note:
 *  For convenience, you can just give up the current time slice.
 */
/*** exercise 4.6 ***/
void sys_yield(void)
{
	bcopy((void *)KERNEL_SP - sizeof(struct Trapframe),
              (void *)TIMESTACK - sizeof(struct Trapframe),
              sizeof(struct Trapframe));
	sched_yield();
}

/* Overview:
 * 	This function is used to destroy the current environment.
 *
 * Pre-Condition:
 * 	The parameter `envid` must be the environment id of a
 * process, which is either a child of the caller of this function
 * or the caller itself.
 *
 * Post-Condition:
 * 	Return 0 on success, < 0 when error occurs.
 */
int sys_env_destroy(int sysno, u_int envid)
{
	/*
		printf("[%08x] exiting gracefully\n", curenv->env_id);
		env_destroy(curenv);
	*/
	int r;
	struct Env *e;

	if ((r = envid2env(envid, &e, 1)) < 0) {
		return r;
	}

	printf("[%08x] destroying %08x\n", curenv->env_id, e->env_id);
	env_destroy(e);
	return 0;
}

/* Overview:
 * 	Set envid's pagefault handler entry point and exception stack.
 *
 * Pre-Condition:
 * 	xstacktop points one byte past exception stack.
 *
 * Post-Condition:
 * 	The envid's pagefault handler will be set to `func` and its
 * 	exception stack will be set to `xstacktop`.
 * 	Returns 0 on success, < 0 on error.
 */
/*** exercise 4.12 ***/
int sys_set_pgfault_handler(int sysno, u_int envid, u_int func, u_int xstacktop)
{
	// Your code here.
	struct Env *env;
	int ret;

	if ((ret = envid2env(envid, &env, 0)) < 0) {
		return ret;
	}
	env->env_pgfault_handler = func;
	env->env_xstacktop = xstacktop;

	return 0;
	//	panic("sys_set_pgfault_handler not implemented");
}

/* Overview:
 * 	Allocate a page of memory and map it at 'va' with permission
 * 'perm' in the address space of 'envid'.
 *
 * 	If a page is already mapped at 'va', that page is unmapped as a
 * side-effect.
 *
 * Pre-Condition:
 * perm -- PTE_V is required,
 *         PTE_COW is not allowed(return -E_INVAL),
 *         other bits are optional.
 *
 * Post-Condition:
 * Return 0 on success, < 0 on error
 *	- va must be < UTOP
 *	- env may modify its own address space or the address space of its children
 */
/*** exercise 4.3 ***/
int sys_mem_alloc(int sysno, u_int envid, u_int va, u_int perm)
{
	// Your code here.
	struct Env *env;
	struct Page *ppage;
	int ret;
	ret = 0;

	if (va >= UTOP) {
		return -E_INVAL;
	}

	if (((perm & PTE_V) == 0) || ((perm & PTE_COW) != 0)) {
		return -E_INVAL;
	}

	if ((ret = page_alloc(&ppage)) != 0) {
		return ret;
	}

	if ((ret = envid2env(envid, &env, 1)) != 0) {
		return ret;
	}

	if ((ret = page_insert(env->env_pgdir, ppage, va, perm)) != 0) {
		return ret;
	}
	return 0;
}

/* Overview:
 * 	Map the page of memory at 'srcva' in srcid's address space
 * at 'dstva' in dstid's address space with permission 'perm'.
 * Perm must have PTE_V to be valid.
 * (Probably we should add a restriction that you can't go from
 * non-writable to writable?)
 *
 * Post-Condition:
 * 	Return 0 on success, < 0 on error.
 *
 * Note:
 * 	Cannot access pages above UTOP.
 */
/*** exercise 4.4 ***/
int sys_mem_map(int sysno, u_int srcid, u_int srcva, u_int dstid, u_int dstva,
				u_int perm)
{
	int ret;
	u_int round_srcva, round_dstva;
	struct Env *srcenv;
	struct Env *dstenv;
	struct Page *ppage;
	Pte *ppte;

	ppage = NULL;
	ret = 0;
	round_srcva = ROUNDDOWN(srcva, BY2PG);
	round_dstva = ROUNDDOWN(dstva, BY2PG);

    //your code here
	if ((srcva >= UTOP) || (dstva >= UTOP)) {
		printf("hhhh srcva || dstva > UTOP\n");
		return -E_INVAL;
	}
	if ((perm & PTE_V) == 0) {
		printf("hhhhh srcva:0x%x,dstva:0x%x,PTE_V\n",srcva,dstva);
		return -E_INVAL;
	}

	if ((ret = envid2env(srcid, &srcenv, 0)) != 0) {
		printf("hhhhhh envid2env\n");
		return ret;
	}
	ppage = page_lookup(srcenv->env_pgdir, round_srcva, &ppte);

	if (ppage == NULL) {
		printf("hhhhhhh ppage==NULL\n");
		return -E_INVAL;
	}
	if ((((*ppte) & PTE_R) == 0) && ((perm & PTE_R) != 0)) {
		printf("hhhhhh PTE_R,,\n");
		return -E_INVAL;
	}

	if ((ret = envid2env(dstid, &dstenv, 0)) != 0) {
		printf("hhhhhhhhh envid2env dstid:%d\n",dstid);
		return ret;
	}
	ret = page_insert(dstenv->env_pgdir, ppage, round_dstva, perm);
	return ret;
}

/* Overview:
 * 	Unmap the page of memory at 'va' in the address space of 'envid'
 * (if no page is mapped, the function silently succeeds)
 *
 * Post-Condition:
 * 	Return 0 on success, < 0 on error.
 *
 * Cannot unmap pages above UTOP.
 */
/*** exercise 4.5 ***/
int sys_mem_unmap(int sysno, u_int envid, u_int va)
{
	// Your code here.
	int ret;
	struct Env *env;
//	struct Page *ppage;

	if (va >= UTOP) {
		return -E_INVAL;
	}

	if ((ret = envid2env(envid, &env, 0)) != 0) {
		return ret;
	}
	page_remove(env->env_pgdir, va);
	return ret;
	//	panic("sys_mem_unmap not implemented");
}

/* Overview:
 * 	Allocate a new environment.
 *
 * Pre-Condition:
 * The new child is left as env_alloc created it, except that
 * status is set to ENV_NOT_RUNNABLE and the register set is copied
 * from the current environment.
 *
 * Post-Condition:
 * 	In the child, the register set is tweaked so sys_env_alloc returns 0.
 * 	Returns envid of new environment, or < 0 on error.
 */
/*** exercise 4.8 ***/
int sys_env_alloc(void)
{
	// Your code here.
	int r;
	struct Env *e;
	
	if ((r = env_alloc(&e, curenv->env_id)) < 0) {
		return r;
	}
	bcopy((void *)KERNEL_SP - sizeof(struct Trapframe), (void *)&(e->env_tf), sizeof(struct Trapframe));
	e->env_tf.pc = e->env_tf.cp0_epc;
	e->env_tf.regs[2] = 0; //return value
	e->env_status = ENV_NOT_RUNNABLE;
	e->env_pri = curenv->env_pri;

    struct TCB *t;
    if (tcb_alloc(e, &t) < 0) { //0号
        return;
    }
    bcopy((void *)KERNEL_SP - sizeof(struct Trapframe), (void *)&(t->tcb_tf), sizeof(struct Trapframe));
    t->tcb_tf.pc = e->env_tf.pc;
    t->tcb_tf.regs[2] = 0; //return value
    t->tcb_status = TCB_NOT_RUNNABLE;
    t->tcb_pri = e->env_pri;

	return e->env_id;
	//	panic("sys_env_alloc not implemented");
}

/* Overview:
 * 	Set envid's env_status to status.
 *
 * Pre-Condition:
 * 	status should be one of `ENV_RUNNABLE`, `ENV_NOT_RUNNABLE` and
 * `ENV_FREE`. Otherwise return -E_INVAL.
 *
 * Post-Condition:
 * 	Returns 0 on success, < 0 on error.
 * 	Return -E_INVAL if status is not a valid status for an environment.
 * 	The status of environment will be set to `status` on success.
 */
/*** exercise 4.14 ***/
int sys_set_env_status(int sysno, u_int envid, u_int status)
{
	// Your code here.
	struct TCB *tcb;
	struct Env *env;
	int ret;
	
	if ((status != TCB_RUNNABLE) && (status != TCB_NOT_RUNNABLE) && (status != TCB_FREE)) {
		return -E_INVAL;
	}

	if ((ret = envid2env(envid, &env, 0)) < 0) {
		return ret;
	}
	tcb = &(env->env_threads[0]);

	if ((tcb->tcb_status != TCB_RUNNABLE) && (status == TCB_RUNNABLE)) {
		LIST_INSERT_HEAD(&tcb_sched_list[0], tcb, tcb_sched_link);
	} else if ((tcb->tcb_status == TCB_RUNNABLE) && (status != TCB_RUNNABLE)) {
		LIST_REMOVE(tcb, tcb_sched_link);
	}
	env->env_status = status;
	tcb->tcb_status = status;

	return 0;
	//	panic("sys_env_set_status not implemented");
}

/* Overview:
 * 	Set envid's trap frame to tf.
 *
 * Pre-Condition:
 * 	`tf` should be valid.
 *
 * Post-Condition:
 * 	Returns 0 on success, < 0 on error.
 * 	Return -E_INVAL if the environment cannot be manipulated.
 *
 * Note: This hasn't be used now?
 */
int sys_set_trapframe(int sysno, u_int envid, struct Trapframe *tf)
{

	return 0;
}

/* Overview:
 * 	Kernel panic with message `msg`.
 *
 * Pre-Condition:
 * 	msg can't be NULL
 *
 * Post-Condition:
 * 	This function will make the whole system stop.
 */
void sys_panic(int sysno, char *msg)
{
	// no page_fault_mode -- we are trying to panic!
	panic("%s", TRUP(msg));
}

/* Overview:
 * 	This function enables caller to receive message from
 * other process. To be more specific, it will flag
 * the current process so that other process could send
 * message to it.
 *
 * Pre-Condition:
 * 	`dstva` is valid (Note: NULL is also a valid value for `dstva`).
 *
 * Post-Condition:
 * 	This syscall will set the current process's status to
 * ENV_NOT_RUNNABLE, giving up cpu.
 */
/*** exercise 4.7 ***/
void sys_ipc_recv(int sysno, u_int dstva)
{
	if (dstva >= UTOP) {
		return;
	}
	curenv->env_ipc_recving = 1;
	curenv->env_ipc_dstva = dstva;
	//curenv->env_status = ENV_NOT_RUNNABLE;
    curtcb->tcb_status = TCB_NOT_RUNNABLE;
    curenv->env_ipc_recv_tcbno = TCBID2TCBNO(curtcb->tcb_id);
	sys_yield();
}

/* Overview:
 * 	Try to send 'value' to the target env 'envid'.
 *
 * 	The send fails with a return value of -E_IPC_NOT_RECV if the
 * target has not requested IPC with sys_ipc_recv.
 * 	Otherwise, the send succeeds, and the target's ipc fields are
 * updated as follows:
 *    env_ipc_recving is set to 0 to block future sends
 *    env_ipc_from is set to the sending envid
 *    env_ipc_value is set to the 'value' parameter
 * 	The target environment is marked runnable again.
 *
 * Post-Condition:
 * 	Return 0 on success, < 0 on error.
 *
 * Hint: the only function you need to call is envid2env.
 */
/*** exercise 4.7 ***/
int sys_ipc_can_send(int sysno, u_int envid, u_int value, u_int srcva,
					 u_int perm)
{

	int r;
	struct Env *e;
	struct Page *p;
	
	if (srcva >= UTOP) {
		return -E_INVAL;
	}
	if ((r = envid2env(envid, &e, 0)) != 0) {
		return r;
	}
	if ((e->env_ipc_recving) == 0) {
		return -E_IPC_NOT_RECV;
	}
	
	e->env_ipc_recving = 0;
	e->env_ipc_from = curenv->env_id;
	e->env_ipc_value = value;
	
	if (srcva != 0) {
		p = page_lookup(curenv->env_pgdir, srcva, NULL);
		if (p == NULL) {
			return -E_INVAL;
		}
		if ((r = page_insert(e->env_pgdir, p, e->env_ipc_dstva, perm)) < 0) {
			return r;
		}
	}
	/*if (srcva != 0) {
		e->env_ipc_perm = perm;
	} else {
		e->env_ipc_perm = 0;
	}*/
	e->env_ipc_perm = perm;
	//e->env_status = ENV_RUNNABLE;
	struct TCB *t = &(e->env_threads[e->env_ipc_recv_tcbno]);
	t->tcb_status = TCB_RUNNABLE;
	return 0;
}
