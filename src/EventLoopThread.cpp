#include "EventLoopThread.h"
#include "EventLoop.h"
#include "Thread.h"
#include "CountDownLatch.h"
#include <functional>

EventLoopThread::EventLoopThread()
    :   latch_(1),
        loop_(NULL),
        thread_(std::bind(&EventLoopThread::threadFunc, this), "EventLoopThread")
{}

EventLoopThread::~EventLoopThread()
{
    if(loop_)
    {
        loop_->quit();
        thread_.join();
    }
}

void EventLoopThread::threadFunc()
{
    EventLoop loop;
    loop_ = &loop;
    latch_.countDown();
    loop.loop();
    loop_ = NULL;
}


EventLoop* EventLoopThread::startLoop()
{
    assert(!thread_.started());
    thread_.start();
    latch_.wait();
    return loop_;
}