#pragma once 
#include <pthread.h>
#include "noncopyable.h"

class MutexLock:noncopyable
{
public:
    MutexLock(){pthread_mutex_init(&mutex, NULL);}
    ~MutexLock()
    {
        pthread_mutex_lock(&mutex);//保证销毁的时候没有任何线程在使用该锁
        pthread_mutex_destroy(&mutex);//linux中，destroy函数只会检查mutex锁的状态
    }
    void lock(){pthread_mutex_lock(&mutex);}
    void unlock(){pthread_mutex_unlock(&mutex);}
    pthread_mutex_t* get(){return &mutex;}
private:
    pthread_mutex_t mutex;
};

class MutexLockGuard:noncopyable
{
public:
    MutexLockGuard(MutexLock& __mutex):mutex(__mutex){mutex.lock();}
    ~MutexLockGuard(){mutex.unlock();}

private:
    MutexLock &mutex;
};

#define MutexLockGuard(x) static_assert(false, "missing mutex guard var name")