#pragma once

#include "EventLoop.h"
#include "noncopyable.h"
#include "Thread.h"
#include "CountDownLatch.h"


class EventLoopThread : noncopyable
{
public:
    EventLoopThread();
    ~EventLoopThread();
    EventLoop* startLoop();


private:
    EventLoop *loop_;//包含的EventLoop对象的指针

    void threadFunc();//EventLoop所在的Thread

    Thread thread_;

    CountDownLatch latch_;


};




