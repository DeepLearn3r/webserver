#include "LogStream.h"
#include <algorithm>
#include <cstdio>

const char digits[] = "9876543210123456789";
const char* zero = digits + 9;

//T类型value转化为字符串类型
template<typename T>
size_t convert(char buf[], T value)
{
    T i = value;
    char* p = buf;
    
    do
    {
        int lsd = static_cast<int>(i%10);
        i/=10;
        *p++ = zero[lsd];
    } while (i!=0);

    if(value<0)
    {
        *p++ = '-';
    }
    *p = '\0';
    std::reverse(buf, p);
    return p-buf;    
}

//存T类型整数进缓冲区
template<typename T>
void LogStream::formatInteger(T value)
{
    // buffer容不下kMaxNumericSize个字符的话会被直接丢弃
    if(buffer_.avail()>=kMaxNumericSize)
    {
        size_t len = convert(buffer_.current(), value);
        buffer_.add(len);
    }
}

LogStream& LogStream::operator<<(int value)
{
    formatInteger(value);
    return *this;
}
LogStream& LogStream::operator<<(short value)
{
    return operator<<(static_cast<int>(value));
}
LogStream& LogStream::operator<<(unsigned short value)
{
    return operator<<(static_cast<unsigned int>(value));
}
LogStream& LogStream::operator<<(unsigned int value)
{
    formatInteger(value);
    return *this;
}
LogStream& LogStream::operator<<(long value)
{
    formatInteger(value);
    return *this;
}
LogStream& LogStream::operator<<(unsigned long value)
{
    formatInteger(value);
    return *this;
}
LogStream& LogStream::operator<<(long long value)
{
    formatInteger(value);
    return *this;
}
LogStream& LogStream::operator<<(unsigned long long value)
{
    formatInteger(value);
    return *this;
}
LogStream& LogStream::operator<<(double value)
{
    if(buffer_.avail()>=kMaxNumericSize)
    {
        int len = snprintf(buffer_.current(), kMaxNumericSize, "%.12g", value);
        buffer_.add(len);
    }
    return *this;
}
LogStream& LogStream::operator<<(long double value)
{
    if(buffer_.avail()>=kMaxNumericSize)
    {
        int len = snprintf(buffer_.current(), kMaxNumericSize, "%.12Lg", value);
        buffer_.add(len);
    }
    return *this;
}