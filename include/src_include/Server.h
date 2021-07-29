#pragma once

#include <memory>
#include "EventLoopThreadPool.h"
#include "Channel.h"

class EventLoop;

class Server
{
public:
    Server(EventLoop* loop, int threadNum, int port);
    // ~Server(){}
    EventLoop* getLoop() const {return loop_;}
    void start();
    void handleNewConn();//如何处理新来的连接
    void handleThisConn();//如何处理自身的连接


private:
    EventLoop* loop_;
    int threadNum_;
    std::unique_ptr<EventLoopThreadPool> threadPool_;

    std::shared_ptr<Channel> acceptChannel_;//listenfd的事件

    int port_;//接收端口号
    int listenFd_;//监听文件描述符

    static const int MAXFDS = 100000;


};