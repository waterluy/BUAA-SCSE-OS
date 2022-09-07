//
// Created by wanglu on 2022/6/29.
// test pthread_create pthread_exit pthread_join
//

#include "lib.h"

void *function1( void *ptr )
{
    char *message;
    message = (char *) ptr;
    writef("%s \n", message);
    char *exit_value = "wl's thread exit success! yeah!\n";
    pthread_exit(exit_value);
    writef("exit error 555555555555~\n");
}

void *function2( void *ptr )
{
    char *message;
    message = (char *) ptr;
    writef("%s \n", message);
    //char *exit_value = "wl's exit success! yeah!\n";
    //pthread_exit(exit_value);
    writef("no exit  oh yeah!~\n");
}

void umain(void)
{
    pthread_t thread1, thread2;
    char *message1 = "Thread 1";
    char *message2 = "Thread 2";
    int  iret1, iret2;

    /* Create independent threads each of which will execute function */


    iret1 = pthread_create( &thread1, NULL, function1, (void*) message1);
    iret2 = pthread_create( &thread2, NULL, function2, (void*) message2);
    //iret3 = pthread_create( &thread3, NULL, function3, (void*) message3);

    /* Wait till threads are complete before main continues. Unless we  */
    /* wait we run the risk of executing an exit which will terminate   */
    /* the process and all threads before the threads have completed.   */

    char *ret1;
    int *ret2;

    pthread_join( thread1, &ret1);
    pthread_join( thread2, &ret2);

    writef("Thread 1 returns: %d, retval: %s !\n",iret1, ret1);
    writef("Thread 2 returns: %d, retval: %d !\n",iret2, *ret2);

    exit(0);
}