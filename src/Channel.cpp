#include "Channel.h"
#include "EventLoop.h"


Channel::Channel(EventLoop* loop, int fd)
    :   loop_(loop),
        fd_(fd),
        events_(0),
        revents_(0),
        lastEvents_(0)
{

}

Channel::Channel(EventLoop* loop)
    :   loop_(loop),
        fd_(0),
        events_(0),
        revents_(0),
        lastEvents_(0)
{

}

Channel::~Channel()
{

}


