#include "FileUtil.h"
#include <string>
#include <cstdio>
// #include <cstring>

//e：表示O_CLOEXEC，即EXEC就关闭
AppendFile::AppendFile(const std::string& filename)
        : fp_(fopen(filename.c_str(), "ae"))
{
    setbuffer(fp_, buffer_, sizeof(buffer_));//设置文件流的缓冲区
}

AppendFile::~AppendFile()
{
    flush();
    fclose(fp_);
}

//调用无锁版本写入，加快速度，但是不是线程安全的
size_t AppendFile::write(const char* logline, const size_t len)
{
    return fwrite_unlocked(logline, 1, len, fp_);//1 表示logline中的元素每个都是1字节大小
}

//由于缓冲区只有8KB
//因此不停的调用write，达到全部写入的目的
//注意错误判断
void AppendFile::append(const char* logline, const size_t len)
{
    size_t n = this->write(logline, len);
    size_t remain = len-n;
    while(remain>0)
    {
        size_t x = this->write(logline+n, remain);
        if(x == 0)
        {
            int err = ferror(fp_);
            if(err)fprintf(stderr, "AppendFile::append() failed !\n");
            break;
        }
        n += x;
        remain = len-n;
    }
}

void AppendFile::flush()
{
    fflush(fp_);
}