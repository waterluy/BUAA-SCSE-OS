#include <pmap.h>
#include <printf.h>
#include <thread.h> 

/* Overview:
 *  Implement simple round-robin scheduling.
 *
 *
 * Hints:
 *  1. The variable which is for counting should be defined as 'static'.
 *  2. Use variable 'env_sched_list', which is a pointer array.
 *  3. CANNOT use `return` statement!
 */
/*** exercise 3.15 ***/
void sched_yield(void)
{
    static int count = 0; // remaining time slices of current env
    static int point = 0; // current env_sched_list index
    
    /*  hint:
     *  1. if (count==0), insert `e` into `env_sched_list[1-point]`
     *     using LIST_REMOVE and LIST_INSERT_TAIL.
     *  2. if (env_sched_list[point] is empty), point = 1 - point;
     *     then search through `env_sched_list[point]` for a runnable env `e`, 
     *     and set count = e->env_pri
     *  3. count--
     *  4. env_run()
     *
     *  functions or macros below may be used (not all):
     *  LIST_INSERT_TAIL, LIST_REMOVE, LIST_FIRST, LIST_EMPTY
     */
    struct TCB *t;

    if ((count <= 0) || (curtcb == NULL) || (curtcb->tcb_status != TCB_RUNNABLE)) {
		count = 0;
		if (curtcb != NULL) {
			LIST_REMOVE(curtcb, tcb_sched_link);
			LIST_INSERT_TAIL(&tcb_sched_list[1-point], curtcb, tcb_sched_link);
		}
		int flag = 0;
		LIST_FOREACH(t, &tcb_sched_list[point], tcb_sched_link) {
			if (t->tcb_status == TCB_RUNNABLE) {
				flag = 1;
				break;
			}
		}
		if (flag == 0) { // 'empty' 
			point = 1 - point;
		}
		LIST_FOREACH(t, &tcb_sched_list[point], tcb_sched_link) {
        	if (t->tcb_status == TCB_RUNNABLE) {
                count = t->tcb_pri;
                count = count - 1;
				tcb_run(t);
                break;
            }
        }
		panic("\n^^^^^^No tcb is RUNNABLE!^^^^^^");
    } else {
		count = count - 1;
		tcb_run(curtcb);
	}
}
