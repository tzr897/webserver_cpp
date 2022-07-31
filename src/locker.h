#ifndef __LOCKER__H
#define __LOCKER__H

#include <pthread.h>
#include <semaphore.h>
#include <exception>

// #include "cond.h"
// #include "sem.h"

// class Locker is used for thread synchronization
class Locker
{
public:
    Locker();

    ~Locker();

    bool lock();

    bool unlock();
    
    pthread_mutex_t* get();

private:
    pthread_mutex_t m_mutex;
};

#endif