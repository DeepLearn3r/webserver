#pragma once

#include "LogStream.h"
#include "noncopyable.h"
#include "Thread.h"
#include "MutexLock.h"
#include "Condition.h"
#include "CountDownLatch.h"
#include <vector>
#include <memory>
#include <string>

/*
append函数作为前端
利用Thread，注册threadFunc做为后端
*/
class AsyncLogging:noncopyable
{
private:
    typedef FixedBuffer<kLargeBuffer> Buffer;
    typedef std::shared_ptr<Buffer> BufferPtr;
    typedef std::vector<BufferPtr> BufferVector;

public:
    AsyncLogging(const std::string& basename, int flushInterval=2);
    ~AsyncLogging()
    {
        if(running_)stop();
    }
    void append(const char* logline, int len);
    void start()
    {
        running_ = true;
        thread_.start();
        latch_.wait();//确保退出的时候，线程已经执行到threadFunc了
    }
    void stop()
    {
        running_ = false;
        //由于要关闭了，告诉后端准备跑路了
        cond_.notify();
        thread_.join();
    }

private:
    void threadFunc();
    bool running_;

    const int flushInterval_;
    std::string basename_;

    Thread thread_;
    MutexLock mutex_;
    Condition cond_;

    BufferPtr currentBuffer_;
    BufferPtr nextBuffer_;
    BufferVector buffers_;
    CountDownLatch latch_;

};