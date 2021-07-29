#include "Logging.h"
#include <string>
#include "AsyncLogging.h"
#include "LogStream.h"
#include <sys/time.h>
#include <time.h>
#include <pthread.h>
#include <memory>

std::string Logger::logFileName_ = "./webserver.log";

static pthread_once_t once_control_ = PTHREAD_ONCE_INIT;
static std::shared_ptr<AsyncLogging> AsyncLogger_;
// static AsyncLogging *AsyncLogger_;

void once_init()
{
    AsyncLogger_.reset(new AsyncLogging(Logger::getLogFileName()));
    // AsyncLogger_ = new AsyncLogging(Logger::getLogFileName());
    AsyncLogger_->start();
}

void output(const char* msg, int len)
{
    //output以前一定要确认一下是否创建实例
    pthread_once(&once_control_, once_init);
    AsyncLogger_->append(msg, len);
}

void Logger::Impl::formatTime()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    time_t value;
    value = tv.tv_sec;
    struct tm* p_tm = localtime(&value);
    char str_t[26] = {0};
    strftime(str_t, 26, "%Y-%m-%d %H:%M:%S\n", p_tm);

    stream_<<str_t;
}

Logger::Impl::Impl(const char* basename, int line)
    :   basename_(basename),
        line_(line),
        stream_()
{
    formatTime();
}

Logger::Logger(const char* basename, int line)
    :   impl_(basename, line)
{}

Logger::~Logger()
{
    impl_.stream_<<" -- "<<impl_.basename_<<":"<<impl_.line_<<'\n';
    const LogStream::Buffer& buf(stream().buffer());
    output(buf.data(), buf.length());
}




