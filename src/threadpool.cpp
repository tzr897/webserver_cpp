#include "threadpool.h"

template<typename T>
Threadpool<T>::Threadpool(int thread_number, int max_requests) : 
m_thread_number(thread_number),
m_max_requests(max_requests),
m_stop(false),
m_threads(NULL) {
    if ((thread_number <= 0) || (max_requests <= 0)) {
        throw std::exception();
    }

    m_threads = new pthread_t[m_thread_number];
    if (!m_threads) {
        throw std::exception();
    }

    // create thread_number threads, and set them as detached threads
    for (int i = 0; i < thread_number; ++i) {
        printf("create the thread %d\n", i);

        if (pthread_create(m_threads + i, NULL, worker, this) != 0) {
            delete[] m_threads;
            throw std::exception();
        }

        if (pthread_detach(m_threads[i])) {
            delete[] m_threads;
            throw std::exception();
        }
    }
}

template<typename T>
Threadpool<T>::~Threadpool() {
    delete[] m_threads;
    m_stop = true;
}

template<typename T>
bool Threadpool<T>::append(T* request) {
    // The queue is shared by all threads, so use locker
    m_queuelocker.lock();

    // cannot append if size of queue is larger than m_max_requests
    if (m_workqueue.size() > m_max_requests) {
        m_queuelocker.unlock();
        return false;
    }
    
    m_workqueue.push_back(request);
    m_queuelocker.unlock();
    m_queuestat.post();
    return true;
}

template<typename T>
void* Threadpool<T>::worker(void* arg) {
    Threadpool *pool = (Threadpool*)arg;
    pool->run();
    return pool;
}

template<typename T>
void Threadpool<T>::run() {
    while (!m_stop) {
        m_queuestat.wait();

        // here we have task
        m_queuelocker.lock();
        // if the queue is empty, continue
        if (m_workqueue.empty()) {
            m_queuelocker.unlock();
            continue;
        }

        // here we have data to process
        T *request = m_workqueue.front();
        m_workqueue.pop_front();

        // after unlocking, we can process it
        m_queuelocker.unlock();
        if (!request) {
            continue;
        }

        // process it, this function is in the task class
        request->process();
    }
}