#include <env.h>
#include <pmap.h>
#include <printf.h>

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
	struct Env *e;
    
    if ((count <= 0) || (curenv == NULL) || (curenv->env_status != ENV_RUNNABLE)) {
		count = 0;
		if (curenv != NULL) {
//			printf("\n^^^insert curenv to another sched_list^^\n");
			LIST_REMOVE(curenv, env_sched_link);
			LIST_INSERT_TAIL(&env_sched_list[1-point], curenv, env_sched_link);
		}
/*		if (LIST_EMPTY(&env_sched_list[point])) {
            printf("\n^^^^^sched_list%d is empty^^\n",point);
			point = 1 - point;
		}*/
		int flag = 0;
		LIST_FOREACH(e, &env_sched_list[point], env_sched_link) {
			if (e->env_status == ENV_RUNNABLE) {
				flag = 1;
//				printf("%d flag = 1  envid:%d pri:%d\n",flag,e->env_id,e->env_pri);
//				printf("\nrunnable, pri: %d\n",e->env_pri);
				break;
			}
		}
		if (flag == 0) { // 'empty' 
//			printf("\naaaaaaaaa   change point %d to %d\n", point, 1-point);
			point = 1 - point;
//			printf("change point %d\n",point);
		}
		LIST_FOREACH(e, &env_sched_list[point], env_sched_link) {
        	if (e->env_status == ENV_RUNNABLE) {
//				printf("\nnext == null? : %d ^ next pri:%d\n",(LIST_NEXT(e, env_sched_link) == NULL), (LIST_NEXT(e, env_sched_link))->env_pri);
                //LIST_REMOVE(e, env_sched_link);
                count = e->env_pri;
                count = count - 1;
		//		printf("\n^^^^point: %d^count:%d^now env pri: %d^envid:%d^^^^^\n",point, count,e->env_pri,e->env_id);
//                printf("sched.c env_run envid:%d\n",e->env_id);
				env_run(e);
		//		panic("\n^^^^^run an env its pri is %d^^^^\n",e->env_pri);
                break;
            }
        }
		panic("\n^^^^^^No env is RUNNABLE!^^^^^^");
    } else {
		count = count - 1;
		//printf("\n^^point: %d^^count: %d^^continue run env pri: %d^^\n",point,count,curenv->env_pri);
		env_run(curenv);
	}
	//env_run(LIST_FIRST(env_sched_list));
}
