#ifndef __SEM__H
#include "sem.h"
#endif


Sem::Sem() {
    if (sem_init(&m_sem, 0, 0) != 0) {
        throw std::exception();
    }
}

Sem::Sem(int num) {
    if (sem_init(&m_sem, 0, num) != 0) {
        throw std::exception();
    }
}

Sem::~Sem() {
    sem_destroy(&m_sem);
}

bool Sem::wait() {
    return sem_wait(&m_sem) == 0;
}

bool Sem::post() {
    return sem_post(&m_sem) == 0;
}