#pragma once

#include <memory>
#include <functional>
#include <sys/epoll.h>

class EventLoop;

class HttpData;

//fd所有者->向Channel注册监听事件、响应函数
//Channel->epoll
//epoll->填充Channel的revents_
//进入EventLoop，执行Channel的handleEvents函数
class Channel
{
private:
    typedef std::function<void()> CallBack;

public:
    Channel(EventLoop* loop);
    Channel(EventLoop* loop, int fd);
    ~Channel();

    //获取、设置监听描述符
    int getFd()const {return fd_;}
    void setFd(int fd){fd_ = fd;}

    //holder
    void setHolder(std::shared_ptr<HttpData> holder){holder_ = holder;}
    std::shared_ptr<HttpData> getHolder()
    {
        std::shared_ptr<HttpData> ret(holder_.lock());
        return ret;
    }

    //注册事件
    void setEvents(__uint32_t events){events_ = events;}
    void setREvents(__uint32_t events){revents_ = events;}
    //返回事件
    __uint32_t& getEvents() {return events_;}
    __uint32_t getLastEvents() const {return lastEvents_;}
    //判断当前事件和上一次事件是否相同
    bool EqualAndUpdateLastEvents()
    {
        bool ret = (lastEvents_ == events_);
        lastEvents_ = events_;
        return ret;
    }

    //注册回调
    void setReadHandler(CallBack&& readHandler){readHandler_ = readHandler;}
    void setWriteHandler(CallBack&& writeHandler){writeHandler_ = writeHandler;}
    void setErrorHandler(CallBack&& errorHandler){errorHandler_ = errorHandler;}
    void setConnHandler(CallBack&& connHandler){connHandler_ = connHandler;}

    //处理事件
    void handleEvents()
    {
        events_ = 0;
        //读写端被关闭，如果有数据读，则继续，没有则返回
        if((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN))
        {
            events_ = 0;
            return;
        }
        if(revents_ & EPOLLERR)
        {
            if(errorHandler_)errorHandler_();
            events_ = 0;
            return;
        }
        //这里的EPOLLRDHUP，应该是想把内核缓冲区中的数据读完
        //于是如果没有数据，则关闭的任务交给readHandler_
        if(revents_ & (EPOLLIN | EPOLLPRI | EPOLLRDHUP))
        {
            handleRead();
        }
        if(revents_ & EPOLLOUT)
        {
            handleWrite();
        }
        handleConn();//向epoll中注册信息的channel事件
    }
    //处理事件
    void handleRead()
    {
        if(readHandler_)readHandler_();
    }
    void handleWrite()
    {
        if(writeHandler_)writeHandler_();
    }
    void handleConn()
    {
        if(connHandler_)connHandler_();
    }
    


private:
    EventLoop* loop_;//所属EventLoop

    int fd_;//注册channel事件的fd

    std::weak_ptr<HttpData> holder_;//哪一个client持有这个channel

    __uint32_t events_;//注册的事件
    __uint32_t revents_;//就绪的事件
    __uint32_t lastEvents_;//上一次注册的事件

    CallBack readHandler_;//处理读事件
    CallBack writeHandler_;//处理写事件
    CallBack errorHandler_;//处理异常或错误
    CallBack connHandler_;//处理连接
};

