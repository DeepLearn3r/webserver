#pragma once 

// #define _GNU_SOURCE
#include <cstdio>
// #include <stdint.h>



/*
定义了四个__thread变量

cachedTid()：线程id、线程id的string版本，线程id长度
tid()：返回线程id
tidStringLength()：返回线程id长度，字节为单位
name()：返回线程姓名
*/
namespace CurrentThread
{
    extern __thread int t_cachedTid;//线程id
    extern __thread char t_tidString[32];//线程id的字符串版本
    extern __thread int t_tidStringLength;//线程id字符串版本的长度
    extern __thread const char* t_threadName;//线程名称

    void cachedTid();
    
    //获得线程id
    inline int tid()
    {
        //一种优化分支预测的函数，
        //这里表示t_cachedTid==0为假的可能性更高，更不专注于if语句内部的语句
        if(__builtin_expect(t_cachedTid==0, 0)) cachedTid();
        return t_cachedTid;
    }

    inline int tidStringLength()
    {
        return t_tidStringLength;
    }

    inline const char* name()
    {
        return t_threadName;
    }
}