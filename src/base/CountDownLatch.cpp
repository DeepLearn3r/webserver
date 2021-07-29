#include "CountDownLatch.h"
#include "MutexLock.h"
#include "Condition.h"
// #include <iostream>
// using namespace std;

//如果不为0，则线程一直阻塞在这里
void CountDownLatch::wait()
{
    // cout<<"Count Down Latch Wait"<<endl;
    MutexLockGuard lock(mutex);
    while(count>0)cond.wait();
}


//如果不为0, 期待再次调用
void CountDownLatch::countDown()
{
    // cout<<"Count Down Latch CountDown"<<endl;
    MutexLockGuard lock(mutex);
    --count;
    if(count<=0)cond.notifyAll();
}
