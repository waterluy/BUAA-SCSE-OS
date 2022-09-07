#include "lib.h"
#include <mmu.h>
#include <env.h>

void exit(void) {
    struct TCB *t;
    u_int tcbid = syscall_get_thread_id(&t);
    t->tcb_exit_value = THREAD_FINISH;
	syscall_thread_destroy(t->tcb_id);
}


struct Env *env;

void
libmain(int argc, char **argv)
{
	// set env to point at our env structure in envs[].
	env = 0;	// Your code here.
	//writef("xxxxxxxxx %x  %x  xxxxxxxxx\n",argc,(int)argv);
	int envid;
	envid = syscall_getenvid();
	envid = ENVX(envid);
	env = &envs[envid];
	// call user main routine
	umain(argc, argv);
	// exit gracefully
	exit();
	//syscall_env_destroy(0);
}
