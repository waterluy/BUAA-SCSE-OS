//
// Created by wanglu on 2022/6/29.
//

#ifndef INC_20373361_LAB4_CHALLENGE_SEMAPHORE_H
#define INC_20373361_LAB4_CHALLENGE_SEMAPHORE_H

#define SEM_VALID 1
#define SEM_INVALID 0

LIST_HEAD(Sem_wait_list, TCB);

typedef struct {
    int sem_value;
    int sem_shared; // 1 shared         0 not shared
    int sem_status;
    int sem_wait_count;
    struct Sem_wait_list sem_wait_list;
    u_int sem_envid;
} sem_t;

#endif //INC_20373361_LAB4_CHALLENGE_SEMAPHORE_H
