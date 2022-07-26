#ifndef __COND__H
#define __COND__H

#include <pthread.h>
#include <semaphore.h>
#include <exception>

// use class Cond to decide whether one thread should be awaked
class Cond
{
public:
    Cond();

    ~Cond();

    bool wait(pthread_mutex_t* mutex);

    bool timewait(pthread_mutex_t* mutex, struct timespec t);

    bool signal();
    
    bool broadcast();

private:
    pthread_cond_t m_cond;
};

#endif