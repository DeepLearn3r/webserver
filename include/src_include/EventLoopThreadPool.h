#pragma once

#include "EventLoop.h"
#include "EventLoopThread.h"
#include "noncopyable.h"
#include <vector>
#include <memory>

class EventLoopThreadPool : noncopyable
{
public:
    EventLoopThreadPool(EventLoop* baseLoop, int numThreads);
    ~EventLoopThreadPool();

    void start();//启动线程池

    EventLoop* getNextLoop();//返回下一个EventLoop（轮询法注册Channel）


private:
    EventLoop* baseLoop_;//baseLoop_，提供给服务器的单独的EventLoop

    std::vector<std::shared_ptr<EventLoopThread>> threads_;//EventLoopThread池，用于监听客户连接
    std::vector<EventLoop*> loops_;//对应threads_中返回的EventLoop指针

    int next_;//对应loops_和threads_的下标
    int numThreads_;//线程池中线程的个数
    bool started_;//线程池是否启动


};