//
// Created by wanglu on 2022/6/29.
//

#include "lib.h"

int sem_init(sem_t *sem, int pshared, unsigned int value) {
    return syscall_sem_init(sem, pshared, value);
}

int sem_destroy(sem_t *sem) {
    return syscall_sem_destroy(sem);
}

int sem_post(sem_t * sem) {
    return syscall_sem_post(sem);
}

int sem_wait(sem_t * sem) {
    return syscall_sem_wait(sem);
}

int sem_trywait(sem_t * sem) {
    return syscall_sem_try_wait(sem);
}

int sem_getvalue(sem_t * sem, int * sval) {
    return syscall_sem_get_value(sem, sval);
}