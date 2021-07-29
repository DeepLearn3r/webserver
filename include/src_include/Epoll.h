#pragma once

#include <sys/epoll.h>
#include <memory>
#include <vector>
#include "Channel.h"
#include "HttpData.h"
#include "Timer.h"

// class HttpData;

//接收事件，进行监听，poll，返回就绪事件
class Epoll
{
public:
    typedef std::shared_ptr<Channel> SP_Channel;

public:
    Epoll();
    ~Epoll();
    void epoll_add(SP_Channel request, int timeout);//添加事件监听
    void epoll_mod(SP_Channel request, int timeout);//修改事件监听
    void epoll_del(SP_Channel request);//删除事件监听
    std::vector<SP_Channel> poll();//epoll_wait
    //根据事件触发的结果，向channel中添加事件
    std::vector<SP_Channel> getEvents(int events_num);

    int getEpollfd() const {return epollfd_;}

    void addTimer(SP_Channel rquest, int timeout);
    void handleExpired(){timerManager_.handleExpiredEvents();}

private:
    int epollfd_;
    static const int MAXFDS = 100000;
    std::vector<epoll_event> events_;//就绪的事件
    std::vector<std::shared_ptr<Channel>> fd2chan_;//fd与channel的一一对应关系，方便查找
    std::vector<std::shared_ptr<HttpData>> fd2http_;//fd与Http的一一对应关系
    
    // std::shared_ptr<Channel> fd2chan_[MAXFDS];//fd与channel的一一对应关系，方便查找
    // std::shared_ptr<HttpData> fd2http_[MAXFDS];//fd与Http的一一对应关系

    TimerManager timerManager_;
};