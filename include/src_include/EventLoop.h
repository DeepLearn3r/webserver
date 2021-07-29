#pragma once

#include <functional>
#include <memory>
#include <vector>
#include <sys/types.h>
#include "Channel.h"
#include "Epoll.h"
#include "MutexLock.h"
#include "CurrentThread.h"
#include <assert.h>


class EventLoop
{

public:
    typedef std::function<void()> Functor;

public:
    EventLoop();
    ~EventLoop();

    void loop();//启动循环
    void quit();//控制循环退出，交给eventLoopThread去处理


    void runInLoop(Functor&& cb);//执行回调
    void queueInLoop(Functor&& cb);//将回调放入容器中，如果满足某些条件，唤醒wakeupFd_

    //检测该eventloop是否在其创建时的线程中
    bool isInThisThread() const {return threadID_==CurrentThread::tid();}
    void assertInThisThread() {assert(isInThisThread());}
    
    //处理事件相关
    void removeFromPoller(std::shared_ptr<Channel> channel)
    {
        poller_->epoll_del(channel);
    }
    void updateToPoller(std::shared_ptr<Channel> channel, int timeout)
    {
        poller_->epoll_mod(channel, timeout);
    }
    void addToPoller(std::shared_ptr<Channel> channel, int timeout)
    {
        poller_->epoll_add(channel, timeout);
    }



private:
    std::shared_ptr<Epoll> poller_;//内部存在的poller

    int wakeupFd_;//用于有事件来的时候通知poller及时处理
    std::shared_ptr<Channel> wakeupChannel_;//wakeupFd_的事件分发器
    void wakeup();//当有非IO复用事件出现时，用于唤醒poller
    void handleRead();//处理wakeupFd_读事件，并更新Channel监听事件
    void handleConn();//在poller中更新Channel事件

    std::vector<Functor> pendingFunctors_;//来的回调函数
    void doPendingFunctors();//处理受阻塞的函数
    bool callingPendingFunctors_;//表示当前是否正在处理回调
    mutable MutexLock mutex_;//锁，用于保护pendingFunctors_（暴露在多线程环境下）

    const pid_t threadID_;//线程id

    bool looping_;//表示当前loop函数是否在执行
    bool quit_;//控制loop函数

};