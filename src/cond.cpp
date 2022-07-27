#include "cond.h"

Cond::Cond() {
    if (pthread_cond_init(&m_cond, NULL) != 0) {
        throw std::exception();
    }
}

Cond::~Cond() {
    pthread_cond_destroy(&m_cond);
}

bool Cond::wait(pthread_mutex_t* mutex) {
    return pthread_cond_wait(&m_cond, mutex) == 0;
}

bool Cond::timewait(pthread_mutex_t* mutex, struct timespec t) {
    return pthread_cond_timedwait(&m_cond, mutex, &t) == 0;
}

bool Cond::signal() {
    return pthread_cond_signal(&m_cond) == 0;
}

bool Cond::broadcast() {
    return pthread_cond_broadcast(&m_cond) == 0;
}