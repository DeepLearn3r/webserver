#pragma once

#include "FileUtil.h"
#include "noncopyable.h"
#include "MutexLock.h"
#include <string>
#include <memory>

/*
FileUtil 的线程安全封装
append：加锁保护的append
append_unlock：直接调用FileUtil::append，每flushEveryN调用FileUtil::flush一次
flush：调用FileUtil::flush一次
*/


class LogFile:noncopyable
{
public:
    LogFile(const std::string& basename, int flushEveryN = 1024)
        :   basename_(basename), 
            flushEveryN_(flushEveryN),
            count_(0),
            mutex_(new MutexLock),
            file_(new AppendFile(basename))
            {

            }
    ~LogFile();

    void append(const char* logline, int len);
    void flush();

private:
    void append_unlock(const char* logline, int len);

    const std::string basename_;
    const int flushEveryN_;

    int count_;
    std::unique_ptr<MutexLock> mutex_;
    std::unique_ptr<AppendFile> file_;//把file_、count_视作临界区

};

