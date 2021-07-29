#pragma once 

#include <memory>
#include <queue>
#include <deque>

class HttpData;

//控制HttpData的生命周期
class TimerNode
{
public:
    TimerNode(std::shared_ptr<HttpData> data, int timeout);
    TimerNode(const TimerNode& tn);
    ~TimerNode();

    void update(int timeout);//更新时间

    bool isValid();//定时器是否有效，如果无效了，设置deleted_=true

    void clear();

    void setDelete(){deleted_ = true;}
    bool isDeleted() const{return deleted_;}

    size_t getExpireTime() const{return expireTime_;}

private:
    bool deleted_;//是否被删除
    size_t expireTime_;//定时时间
    std::shared_ptr<HttpData> SPHttpData;//信息

};


struct TimerCmp
{
    /* data */
    bool operator()(std::shared_ptr<TimerNode>& a, std::shared_ptr<TimerNode>& b)
    {
        return a->getExpireTime() > b->getExpireTime();
    }
};


class TimerManager
{
public:
    TimerManager() = default;
    ~TimerManager() = default;
    void addTimer(std::shared_ptr<HttpData> data, int timeout);
    void handleExpiredEvents();

private:
    using SPTimerNode = std::shared_ptr<TimerNode>;
    //小顶堆
    std::priority_queue<SPTimerNode, std::deque<SPTimerNode>, TimerCmp> timerQ_;


};