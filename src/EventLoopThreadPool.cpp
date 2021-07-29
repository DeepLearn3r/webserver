#include "EventLoopThreadPool.h"
#include "EventLoop.h"
#include "EventLoopThread.h"
#include "Logging.h"
#include <cstdlib>
#include <memory>

EventLoopThreadPool::EventLoopThreadPool(EventLoop* baseLoop, int numThreads)
    :   started_(false),
        next_(0),
        numThreads_(numThreads),
        baseLoop_(baseLoop),
        threads_(numThreads),
        loops_(numThreads)
{
    if(numThreads<=0)
    {
        LOG<<"numThreads <= 0";
        abort();
    }
}

EventLoopThreadPool::~EventLoopThreadPool()
{
    LOG<<"~EventLoopThreadPool()";
}

void EventLoopThreadPool::start()
{
    baseLoop_->assertInThisThread();
    started_ = true;
    for(int i = 0 ;i<numThreads_;++i)
    {
        std::shared_ptr<EventLoopThread> elt(new EventLoopThread());
        threads_[i] = elt;
        loops_[i] = elt->startLoop();
    }
}

EventLoop* EventLoopThreadPool::getNextLoop()
{
    baseLoop_->assertInThisThread();
    assert(started_);
    EventLoop *loop = baseLoop_;
    if(!loops_.empty())
    {
        loop = loops_[next_];
        next_ = (next_+1)%numThreads_;
    }
    if(loop == baseLoop_)
        LOG<<"return  baseLoop_";
    return loop;
}