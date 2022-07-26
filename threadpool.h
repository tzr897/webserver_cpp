#ifndef __THREADPOOL__H
#define __THREADPOOL__H

#include <pthread.h>
#include <exception>
#include <list>
#include <cstdio>

#include "locker.h"
#include "cond.h"
#include "sem.h"

#define THREAD_NUM 8
#define MAX_REQUESTS 10000

// class of thread pool
// use template to design
template<typename T>
class Threadpool
{
public:
    Threadpool(int thread_number = 8, int max_requests = 10000);

    ~Threadpool();

    bool append(T* request);

private:
    // working function of the working thread
    // it excecutes a task from the queue
    static void* worker(void* arg);

    // helper function
    void run();

private:
    // number of all threads
    int m_thread_number;

    // array for thread pool, size: m_thread_number
    pthread_t* m_threads;

    // the maximum number of requests in the queue
    int m_max_requests;

    // work queue
    std::list<T*> m_workqueue;
    
    // the locker for protecting work queue
    Locker m_queuelocker;

    // semaphore for detecting whether there is any task
    Sem m_queuestat;

    // whether to end the thread
    bool m_stop;
};

#endif