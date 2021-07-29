#pragma once
#include "noncopyable.h"
#include "MutexLock.h"
#include <pthread.h>
#include <time.h>
#include <errno.h>

class Condition:noncopyable
{
public:
    Condition(MutexLock& __mutex):mutex(__mutex){pthread_cond_init(&cond, NULL);}
    ~Condition(){pthread_cond_destroy(&cond);}
    void wait(){pthread_cond_wait(&cond, mutex.get());}
    void notify(){pthread_cond_signal(&cond);}
    void notifyAll(){pthread_cond_broadcast(&cond);}
    //等待seconds秒后，返回
    bool waitForSeconds(int seconds)
    {
        struct timespec abstime;
        clock_gettime(CLOCK_REALTIME, &abstime);//<time.h>
        abstime.tv_sec += static_cast<time_t>(seconds);
        return ETIMEDOUT == pthread_cond_timedwait(&cond, mutex.get(), &abstime);
    }

private:
    MutexLock& mutex;
    pthread_cond_t cond;
};