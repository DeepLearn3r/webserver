#include "AsyncLogging.h"
#include "LogFile.h"
#include <string>
#include <functional>
#include <cassert>
#include <utility> //std::move


AsyncLogging::AsyncLogging(const std::string& basename, int flushInterval)
            :   basename_(basename),
                flushInterval_(flushInterval),
                running_(false),
                thread_(std::bind(&AsyncLogging::threadFunc, this), "Logging"),
                mutex_(),
                cond_(mutex_),
                currentBuffer_(new Buffer),
                nextBuffer_(new Buffer),
                latch_(1)//注意这里的latch中的count是1
{
    assert(basename_.size()>1);
    currentBuffer_->bzero();
    nextBuffer_->bzero();
    buffers_.reserve(16);//为什么这么设计？
}

//前端，直接往里扔
//如果满了，就先用nextBuffer，不够再申请
void AsyncLogging::append(const char* logline, int len)
{
    MutexLockGuard lock(mutex_);
    if(currentBuffer_->avail() > len)
    {
        currentBuffer_->append(logline, len);
    }
    else
    {
        buffers_.push_back(currentBuffer_);
        currentBuffer_.reset();
        if(nextBuffer_)
        {
            currentBuffer_ = std::move(nextBuffer_);
        }
        else
        {
            currentBuffer_.reset(new Buffer);
        }
        currentBuffer_->append(logline, len);
        cond_.notify();//告诉后端线程，我们有客人了
    }
}

/*
注册一个写文件的LogFile
准备两个备用的Buffer: newBuffer1, newBuffer2
准备一个用于写的BufferVec: buffersToWrite

将前端的buffers与buffersTowrite交换内容
如果日志太多，只留两个
一个缓冲区一个缓冲区地写入日志中

丢弃non-bzero-ed buffers
*/
void AsyncLogging::threadFunc()
{
    assert(running_ == true);
    latch_.countDown();

    LogFile output(basename_);//写入文件

    //备用缓冲
    BufferPtr newBuffer1(new Buffer);
    BufferPtr newBuffer2(new Buffer);
    newBuffer1->bzero();
    newBuffer2->bzero();
    BufferVector buffersToWrite;
    buffersToWrite.reserve(16);

    while(running_)
    {
        //起始状态，必须要有这两个缓冲，必须要有缓冲vector
        assert(newBuffer1 && newBuffer1->length() == 0);
        assert(newBuffer2 && newBuffer2->length() == 0);
        assert(buffersToWrite.empty());

        //将buffers和currentBuffer中的内容转移到bufferToWrite中
        {
            MutexLockGuard lock(mutex_);
            if(buffers_.empty())
            {
                cond_.waitForSeconds(flushInterval_);
            }
            buffers_.push_back(currentBuffer_);
            currentBuffer_.reset();

            currentBuffer_ = std::move(newBuffer1);
            buffersToWrite.swap(buffers_);
            if(!nextBuffer_)
            {
                nextBuffer_ = std::move(newBuffer2);
            }
        }

        assert(!buffersToWrite.empty());

        //由于4*25 = 100，如果超过100MB，可能就无法在1秒内写入内存了
        //或者说，短时间大量出错日志会出现，往往只有前几个比较重要
        if(buffersToWrite.size()>25)
        {
            buffersToWrite.erase(buffersToWrite.begin()+2, buffersToWrite.end());
        }

        //将日志填入内存
        for(int i = 0;i<buffersToWrite.size();++i)
        {
            output.append(buffersToWrite[i]->data(), buffersToWrite[i]->length());
        }

        //丢弃后面再申请的buffer，即non-bzero-ed buffers
        if(buffersToWrite.size()>2)
        {
            buffersToWrite.resize(2);
        }

        //重新填充newBuffer1、newBuffer2
        if(!newBuffer1)
        {
            assert(!buffersToWrite.empty());
            newBuffer1 = buffersToWrite.back();
            buffersToWrite.pop_back();
            newBuffer1->reset();
        }
        if(!newBuffer2)
        {
            assert(!buffersToWrite.empty());
            newBuffer2 = buffersToWrite.back();
            buffersToWrite.pop_back();
            newBuffer2->reset();
        }


        buffersToWrite.clear();
        output.flush();
    }
    output.flush();

}







