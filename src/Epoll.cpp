#include "Epoll.h"
#include <sys/epoll.h>
#include <memory>
#include <vector>
#include "Channel.h"
#include <cassert>
#include <unistd.h>
#include <cstdio>
#include "Logging.h"
#include "HttpData.h"
#include "Timer.h"

const int MAXEVENTS = 4096;//如何确定
const int EPOLLWAIT_TIME = 10000;//10s

Epoll::Epoll()
    :   epollfd_(epoll_create1(EPOLL_CLOEXEC)), events_(MAXEVENTS),
        fd2chan_(MAXFDS),
        fd2http_(MAXFDS)
{
    assert(epollfd_>0);
}

Epoll::~Epoll()
{
    close(epollfd_);
}

void Epoll::epoll_add(Epoll::SP_Channel request, int timeout)
{
    int fd = request->getFd();
    if(timeout>0)
    {
        this->addTimer(request, timeout);
    }

    struct epoll_event event;
    event.data.fd = fd;
    event.events = request->getEvents();

    fd2http_[fd] = request->getHolder();
    fd2chan_[fd]=request;

    request->EqualAndUpdateLastEvents();//update

    if(epoll_ctl(epollfd_, EPOLL_CTL_ADD, fd, &event)<0)
    {
        perror("epoll_add error");
        // fd2chan_[fd].reset();
    }
}

void Epoll::epoll_mod(Epoll::SP_Channel request, int timeout)
{
    int fd = request->getFd();
    if(timeout>0)
    {
        this->addTimer(request, timeout);
    }
    //如果上一次和这一次的一样，那么就不更改
    //因此不能注册EPOLLONESHOT
    //反之则更改
    if(!request->EqualAndUpdateLastEvents())
    {
        struct epoll_event event;
        event.data.fd = fd;
        event.events = request->getEvents();
        
        if(epoll_ctl(epollfd_, EPOLL_CTL_MOD, fd,  &event)<0)
        {
            perror("epoll_mod error");
            // fd2chan_[fd].reset();
            // fd2http_[fd].reset();
        }
    }
}

void Epoll::epoll_del(Epoll::SP_Channel request)
{
    int fd = request->getFd();
    if(epoll_ctl(epollfd_, EPOLL_CTL_DEL, fd, NULL)<0)
    {
        perror("epoll_del error");
        return;
    }
    fd2chan_[fd].reset();
    fd2http_[fd].reset();
}

std::vector<Epoll::SP_Channel> Epoll::getEvents(int events_num)
{
    if(events_num<=0)return {};
    std::vector<SP_Channel> res;
    for(int i = 0;i<events_num;++i)
    {
        int fd = events_[i].data.fd;

        SP_Channel cur = fd2chan_[fd];

        if(cur)
        {
            cur->setREvents(events_[i].events);//将就绪事件给channel
            cur->setEvents(0);//设置监听事件为0
            res.push_back(cur);
        }
        else{
            LOG<<"SP_Channel cur ("<<fd<<") is invalid.";
        }
    }
    return res;
}

std::vector<Epoll::SP_Channel> Epoll::poll()
{
    //保证有事件返回
    //注意写法
    int events_num = epoll_wait(epollfd_, &*events_.begin(), MAXEVENTS, EPOLLWAIT_TIME);
    if(events_num<0)perror("epoll_wait error");
    std::vector<SP_Channel> res = getEvents(events_num);
    // if(events_num>0)return res;
    return res;
}

void Epoll::addTimer(Epoll::SP_Channel request, int timeout)
{
    std::shared_ptr<HttpData> ret = request->getHolder();
    if(ret)timerManager_.addTimer(ret, timeout);
    else
    {
        LOG<<"error: timer add failed";
    }
}