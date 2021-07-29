#include "Thread.h"
#include "CurrentThread.h"
#include <string>
#include <sys/types.h>
#include <sys/prctl.h>
#include <cassert>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>
// #include <iostream>
// using namespace std;

//Thread.cpp中的__thread全局变量
namespace CurrentThread
{
    __thread int t_cachedTid = 0;
    __thread char t_tidString[32];
    __thread int t_tidStringLength = 6;
    __thread const char* t_threadName = "default";
}

//得到线程的pid
pid_t gettid(){return static_cast<pid_t>(syscall(__NR_gettid));}//unistd

void CurrentThread::cachedTid()
{
    if(t_cachedTid == 0)
    {
        t_cachedTid = gettid();
        t_tidStringLength = snprintf(t_tidString, sizeof(t_tidString), "%5d", t_cachedTid);
    }
}

/*
ThreadData: storage for Thread Data

线程执行函数
线程名字
线程标识
倒计时变量
线程回调函数中应该执行的函数
*/
struct ThreadData
{
    typedef Thread::ThreadFunc ThreadFunc;
    ThreadFunc func_;
    std::string name_;
    pid_t* tid_;
    CountDownLatch* latch_;

    ThreadData(const ThreadFunc& func, const std::string& name, 
    pid_t* tid, CountDownLatch*latch)
                    : func_(func), name_(name), tid_(tid), latch_(latch)
                    {}
    
    //获得当前线程标识
    //进行倒计时
    //给当前线程起名字
    //起名为“finished”
    void runInThread()
    {
        *tid_ = CurrentThread::tid();
        tid_ = NULL;//主要是给Thread类对象知晓当前id即可，用完即扔
        //不知道意欲为何：为了让Thread::start在获取tid_之前，线程已经拿到了tid_
        //即，让Thread::start在退出之前，保证能拿到上面返回的tid_
        //如若不然，那么有可能在Thread对象析构的时候，都拿不到
        latch_->countDown();
        latch_ = NULL;

        CurrentThread::t_threadName = name_.empty()?"Thread":name_.c_str();
        prctl(PR_SET_NAME, CurrentThread::t_threadName);
        func_();
        CurrentThread::t_threadName = "finished";
    }
};


//线程回调函数
void* startThread(void* obj)
{
    ThreadData *data = static_cast<ThreadData*>(obj);
    data->runInThread();
    delete data;
    return NULL;
}

void Thread::setDefaultName()
{
    if(name_.empty())
    {
        name_ = "Thread";
    }
}

//设置线程名字
Thread::Thread(ThreadFunc func, const std::string& name)
            : name_(name), func_(func), started_(false), joined_(false), id_(0), tid_(0), latch_(1)
            {
                setDefaultName();
            }

//析构，如果线程启动了但是还没有结束，将他脱离
Thread::~Thread()
{
    if(started_&& !joined_)pthread_detach(id_);
}

/*
创建ThreadData
把ThreadData传给startThread
注册线程
利用“倒计时”同步方式，获得线程id
*/
void Thread::start()
{
    assert(!started_);//启动之前必须是started=false
    started_ = true;
    ThreadData* data = new ThreadData(func_, name_, &tid_, &latch_);
    if(pthread_create(&id_,NULL,&startThread,data))
    {
        started_ = false;//启动失败
        delete data;
    }
    else
    {
        //启动成功，现在线程已经在运行了
        latch_.wait();//保证Thread对象能拿到tid
        assert(tid_>0);
    }
}

//等待线程结束
int Thread::join()
{
    assert(started_);
    assert(!joined_);
    joined_ = true;
    // cout<<"join"<<endl;
    return pthread_join(id_, NULL);
}
