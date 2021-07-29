#pragma once
#include "CountDownLatch.h"
#include "noncopyable.h"
#include <functional>
#include <sys/types.h>
#include <string>
#include <pthread.h>

/*
主要流程：
Thread被创建，
调用start函数，
start会利用Thread中的信息来创建ThreadData对象data，并生成注册函数startThread，
startThread中，调用data中定义的函数runInThread来最终执行func_()

整个过程利用latch_进行同步，保证线程获得tid_
*/
class Thread : noncopyable
{
public:
    typedef std::function<void()> ThreadFunc;//回调函数
public:
    explicit Thread(ThreadFunc func, const std::string& name = std::string());
    ~Thread();
    void start();//生成ThreadData，注册回调函数，生成线程
    int join();//结束线程
    bool started() const{return started_;}//是否已经开始了
    pid_t tid() const{return tid_;}
    const std::string& name() const{return name_;}


private:
    void setDefaultName();//默认线程名字

    ThreadFunc func_;
    bool started_;
    bool joined_;
    pthread_t id_;//线程id
    pid_t tid_;//线程内核标识
    std::string name_;
    CountDownLatch latch_;//倒计时

};