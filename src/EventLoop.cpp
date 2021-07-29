#include "EventLoop.h"
#include "Logging.h"
#include "CurrentThread.h"
#include "Utils.h"
#include "MutexLock.h"
#include <cstdlib>
#include <sys/eventfd.h>
#include <functional>
#include <sys/epoll.h>
#include <utility>
#include <assert.h>
#include <vector>
#include <unistd.h>

// #include <iostream>

//标识一个线程只能创建一个EventLoop
__thread EventLoop* t_loopInThisThread = 0;

int createEventFd()
{
    int evtfd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
    if(evtfd<0)
    {
        LOG<<"eventfd create failed";
        abort();
    }
}

EventLoop::EventLoop()
    :   poller_(new Epoll()),
        wakeupFd_(createEventFd()),
        wakeupChannel_(new Channel(this, wakeupFd_)),
        callingPendingFunctors_(false),
        mutex_(),
        threadID_(CurrentThread::tid()),
        looping_(false),
        quit_(false)
{
    if(t_loopInThisThread)
    {
        LOG<<"Another EventLoop has already hold this eventloop int thread"<<threadID_;
    }
    else
    {
        t_loopInThisThread = this;
    }
    //注册事件、事件处理函数
    wakeupChannel_->setEvents(EPOLLIN | EPOLLET);
    wakeupChannel_->setReadHandler(std::bind(&EventLoop::handleRead, this));
    wakeupChannel_->setConnHandler(std::bind(&EventLoop::handleConn, this));
    //将wakeupChannel_添加到poller中，
    //不需要定时器，因为wakeupChannel_是跟随EventLoop的随从，
    //它的生命周期约为EventLoop的生命周期
    this->addToPoller(wakeupChannel_, 0);
}

EventLoop::~EventLoop()
{
    close(wakeupFd_);
    t_loopInThisThread = NULL;
}

void EventLoop::wakeup()
{
    //eventfd的缓冲区大小不能低于8字节
    __uint64_t one = 1;
    ssize_t n = writen(wakeupFd_, &one, sizeof(one));
    if(n != sizeof(one))
    {
        LOG << "EventLoop::wakeup() writes " << n << " bytes instead of 8";
    }
}

void EventLoop::handleRead()
{
    __uint64_t one  = 1;
    ssize_t n = readn(wakeupFd_, &one, sizeof(one));
    if(n!=sizeof(one))
    {
        LOG<<"EventLoop::handleRead() reads "<<n<<" bytes instead of 8";
    }
    //读完之后重新填充监听事件
    wakeupChannel_->setEvents(EPOLLIN | EPOLLET);
}

//重新注册wakeupChannel_的事件
void EventLoop::handleConn()
{
    this->updateToPoller(wakeupChannel_, 0);
}


void EventLoop::runInLoop(Functor&& cb)
{
    if(isInThisThread()) cb();
    else queueInLoop(std::move(cb));
}

void EventLoop::queueInLoop(Functor&& cb)
{
    {
        MutexLockGuard lock(mutex_);
        pendingFunctors_.emplace_back(cb);
    }
    //不在本线程中 或 正在处理PendingFunctors时，需要唤醒
    if(!isInThisThread() || callingPendingFunctors_)wakeup();
}

void EventLoop::doPendingFunctors()
{
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;

    {
        MutexLockGuard lock(mutex_);
        functors.swap(pendingFunctors_);
    }
    for(auto& func:functors)
    {
        func();
    }
        
    callingPendingFunctors_ = false;
}

void EventLoop::loop()
{
    assert(!looping_);
    assertInThisThread();
    looping_ = true;
    quit_ = false;
    std::vector<std::shared_ptr<Channel>> ret;
    while(!quit_)
    {
        ret.clear();
        ret = poller_->poll();
        if(!ret.empty())
        {
            // LOG<<CurrentThread::tid()<<" is looping:\t"<<std::to_string(ret.size());
             for(auto& r:ret)
            {
                r->handleEvents();
            }
        }
        doPendingFunctors();
        poller_->handleExpired();
        
    }
    looping_ = false;
}

void EventLoop::quit()
{
    quit_ = true;
    if(!isInThisThread()) wakeup();
}

