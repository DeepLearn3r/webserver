#pragma once
#include "noncopyable.h"
#include <cstring>
#include <string>



const int kSmallBuffer = 4000;//为什么这么设计？4KB
const int kLargeBuffer = 4000 * 1000;//为什么这么设计？4MB


//包装的一个缓冲区
template<int SIZE>
class FixedBuffer:noncopyable
{
public:
    FixedBuffer():cur_(data_){}
    ~FixedBuffer(){}

    //如果缓冲区还有余韵，那么就将len字节的buf传给缓冲区
    void append(const char* buf, size_t len)
    {
        if(avail()>static_cast<int>(len))
        {
            memcpy(cur_, buf, len);//cstring
            cur_ += len;
        }
    }

    const char* data() const{return data_;}//返回缓冲区首部
    int length() const{return static_cast<int>(cur_-data_);}//返回缓冲区用了多少字节空间
    
    char* current() const{return cur_;}//返回缓冲区当前位置
    int avail() const{return static_cast<int>(end()-cur_);}//返回缓冲区还剩多少字节空间
    void add(size_t len)
    {
        //将cur_移动len个字节
        cur_+=len;
    }

    void reset(){cur_ = data_;}//缓冲区复位
    void bzero(){memset(data_, 0, sizeof(data_));}//将缓冲区初始化

private:
    const char* end() const{return data_+sizeof(data_);}

    char data_[SIZE];
    char* cur_;

};



/*
格式化输出
主要做的就是重载<<，使得各个类型的输入都可以变为字符串放进缓冲区buffer_
*/
class LogStream:noncopyable
{
public:
    typedef FixedBuffer<kSmallBuffer> Buffer;

public:
    //布尔
    LogStream& operator<<(bool v)
    {
        buffer_.append(v?"1":"0", 1);
        return *this;
    }

    //整数
    LogStream& operator<<(short);
    LogStream& operator<<(unsigned short);
    LogStream& operator<<(int);
    LogStream& operator<<(unsigned int);
    LogStream& operator<<(long);
    LogStream& operator<<(unsigned long);
    LogStream& operator<<(long long);
    LogStream& operator<<(unsigned long long);


    LogStream& operator<<(const void*);

    //浮点型
    LogStream& operator<<(float value)
    {
        return operator<<(static_cast<double>(value));
    }
    LogStream& operator<<(double);
    LogStream& operator<<(long double);

    //字符型
    LogStream& operator<<(char value)
    {
        buffer_.append(&value, 1);
        return *this;
    }
    LogStream& operator<<(const char* str)
    {
        if(str)buffer_.append(str, strlen(str));
        else buffer_.append("(null)", 6);
        return *this;
    }
    LogStream& operator<<(const unsigned char* str)
    {
        return operator<<(reinterpret_cast<const char*>(str));
    }
    LogStream& operator<<(const std::string& str)
    {
        if(!str.empty()) buffer_.append(str.c_str(), str.length());
        return *this;
    }

    void append(const char* data, int len){buffer_.append(data, len);}
    const Buffer& buffer() const{return buffer_;}
    void resetBuffer(){buffer_.reset();}

private:
    template<typename T>
    void formatInteger(T);//把int型数据放进buffer

    Buffer buffer_;

    static const int kMaxNumericSize = 32;

};