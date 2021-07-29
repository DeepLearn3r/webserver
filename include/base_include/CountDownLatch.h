#pragma once
#include "MutexLock.h"
#include "Condition.h"
#include "noncopyable.h"

/*
倒计时CountDownLatch

CountDownLatch(int count)：count表示需要倒计时的次数
wait()：阻塞，直到count为0
countDown()：倒计时，每调用一次，就count就减少1

*/

class CountDownLatch:noncopyable
{
public:
    //注意构造函数顺序，mutex一定要先于cond构造才行
    explicit CountDownLatch(int __count):count(__count), mutex(), cond(mutex){}
    void wait();
    void countDown();
private:
    int count;
    mutable MutexLock mutex;
    Condition cond;

};