#pragma once

#include <memory>
#include <string>
#include <unistd.h>
#include "Channel.h"
// #include "EventLoop.h"
#include "HttpParse.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "Timer.h"
#include "CountDownLatch.h"
// #include "Logging.h"


class EventLoop;

class HttpData : public std::enable_shared_from_this<HttpData>
{
public:
    enum ConnState
    {
        H_CONNECTED = 0,
        H_DISCONNECTING,
        H_DISCONNECTED
    };


public:
    HttpData(EventLoop* loop, int connfd);
    ~HttpData()
    {
        latch_.wait();
        // LOG<<"close HttpData";
        close(fd_);
    }

    void reset();
    void resetParser();
    std::shared_ptr<Channel> getChannel(){return clientChannel_;}
    EventLoop* getLoop(){return loop_;}

    void newEvent();//给上层建筑提供注册事件的接口
    void handleClose();//处理connfd关闭事件

    void seperateTimer();//分离定时器
    void linkTimer(std::shared_ptr<TimerNode> timer){timer_ = timer;}//给HttpData连接定时器

private:
    EventLoop* loop_;//注册channel_
    int fd_;//对应的客户fd
    std::shared_ptr<Channel> clientChannel_;//httpdata事件表

    std::string inBuffer_;//输入缓冲区
    std::string outBuffer_;//输出缓冲区
    int checked_index_;

    bool error_;//是否出错，400、500错误均关闭链接
    ConnState connectionState_;//控制在办关闭状态下，服务器继续传剩余数据

    HttpRequestParser::PARSE_STATE parser_;
    HttpRequest request_;//请求报文中的信息
    HttpResponse response_;//用于生成响应报文

    std::weak_ptr<TimerNode> timer_;//定时器


    void handleRead();//读
    void handleWrite();//写
    void handleConn();//处理链接

    void handleError(char* msg, int status, std::string&& status_code);

    void fillResponse();//往outBuffer添加文件

    bool isKeepAlive;

    CountDownLatch latch_;
};