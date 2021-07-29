#pragma once

/*
利用stdio的FILE操作，写入
*/
#include <string>
#include "noncopyable.h"

/*
作用是，将长度为len的logline写入文件中
append(): 调用低层的write函数，将logline通过缓冲buffer_写入文件
write()：将不多于buffer的logline写入文件
flush()：将缓冲区清入文件
*/


class AppendFile:noncopyable
{
public:
    explicit AppendFile(const std::string& filename);
    ~AppendFile();
    void append(const char* logline, const size_t len);//写入多个logline，总长度len字节
    void flush();//把缓冲区中的内容刷进内存

private:
    size_t write(const char* logline, const size_t len);//写入
    FILE* fp_;//文件操作
    char buffer_[64*1024];//缓冲区，8KB

};