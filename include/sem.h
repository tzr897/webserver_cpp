#ifndef __SEM__H
#define __SEM__H

#include <pthread.h>
#include <semaphore.h>
#include <exception>

// class Sem is for semaphore
class Sem
{
public:
    Sem();

    Sem(int num);

    ~Sem();

    bool wait();
    
    bool post();

private:
    sem_t m_sem;
};

#endif