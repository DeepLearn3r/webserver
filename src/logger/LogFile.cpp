#include "LogFile.h"
#include "FileUtil.h"
#include "MutexLock.h"
#include <memory>
// #include <iostream>
// using namespace std;

LogFile::~LogFile()
{
    
}

void LogFile::append(const char* logline, int len)
{
    MutexLockGuard lock(*mutex_);
    // cout<<"hello"<<endl;
    this->append_unlock(logline, len);
}

void LogFile::append_unlock(const char* logline, int len)
{
    file_->append(logline, len);
    ++count_;
    if(count_>=flushEveryN_)
    {
        count_=0;
        file_->flush();
    }
}

void LogFile::flush()
{
    MutexLockGuard lock(*mutex_);
    file_->flush();
}
