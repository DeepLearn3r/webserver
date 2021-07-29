#include "Timer.h"
#include "HttpData.h"
#include "Logging.h"
#include "EventLoop.h"
#include <memory>
#include <sys/time.h>
#include <memory>
#include <queue>
#include <deque>



size_t getCurrentTime()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec*1000)+(tv.tv_usec/1000);
}


//上层调用保证timeout不小于0
TimerNode::TimerNode(std::shared_ptr<HttpData> data, int timeout)
    :   SPHttpData(data),
        deleted_(false),
        expireTime_(getCurrentTime()+timeout)
{}

//增加deleted_=false的初始化
TimerNode::TimerNode(const TimerNode& tn)
    :   SPHttpData(tn.SPHttpData),
        expireTime_(0),
        deleted_(false)
{}

//定时器结束，所属的Httpdata也要死
TimerNode::~TimerNode()
{
    if(SPHttpData)
    {
        LOG<<"Time up, handleClose";
        // SPHttpData->seperateTimer();
        SPHttpData->getLoop()->queueInLoop(std::bind(&HttpData::handleClose, SPHttpData));
    }
}

//注意不能是已经deleted_的
//且timeout不能小于0
void TimerNode::update(int timeout)
{
    expireTime_ = getCurrentTime()+timeout;
}

bool TimerNode::isValid()
{
    if(expireTime_>getCurrentTime()) return true;
    else
    {
        deleted_ = true;
        return false;
    }
}

//直接重置httpdata
void TimerNode::clear()
{
    SPHttpData.reset();
    this->setDelete();
}

void TimerManager::addTimer(std::shared_ptr<HttpData> data, int timeout)
{
    SPTimerNode timer(new TimerNode(data, timeout));
    data->linkTimer(timer);//在这里让HttpData连接定时器
    timerQ_.push(timer);
}

void TimerManager::handleExpiredEvents()
{
    while(!timerQ_.empty())
    {
        SPTimerNode timer = timerQ_.top();
        if(timer->isDeleted())
        {
            timerQ_.pop();
        }
        else if(!timer->isValid())
        {
            timerQ_.pop();
        }
        else break;
    }
}

