#include <sys/epoll.h>
#include <memory>
#include <string>
#include <cstring>
#include <unistd.h>
#include <functional>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <iostream>
#include "Channel.h"
#include "EventLoop.h"
#include "HttpParse.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "Utils.h"
#include "HttpData.h"
#include "Logging.h"
#include "CountDownLatch.h"

const int DEFAULT_TIMEOUT = 60 * 1000;//只持续10s
const int DEFAULT_TIMEOUT_SHORT = 2000;//只等待2s

char NOT_FOUND_PAGE[] = "<html>\n"
                        "<head><title>404 Not Found</title></head>\n"
                        "<body bgcolor=\"white\">\n"
                        "<center><h1>404 Not Found</h1></center>\n"
                        "<hr><center>CJJ WebServer/2.0 (Linux)</center>\n"
                        "</body>\n"
                        "</html>";

char FORBIDDEN_PAGE[] = "<html>\n"
                        "<head><title>403 Forbidden</title></head>\n"
                        "<body bgcolor=\"white\">\n"
                        "<center><h1>403 Forbidden</h1></center>\n"
                        "<hr><center>CJJ WebServer/2.0 (Linux)</center>\n"
                        "</body>\n"
                        "</html>";

char BAD_REQUEST_PAGE[] = "<html>\n"
                        "<head><title>400 Bad Request</title></head>\n"
                        "<body bgcolor=\"white\">\n"
                        "<center><h1>400 Bad Request</h1></center>\n"
                        "<hr><center>CJJ WebServer/2.0 (Linux)</center>\n"
                        "</body>\n"
                        "</html>";


char INTERNAL_ERROR_PAGE[] = "<html>\n"
                        "<head><title>500 Internal Error</title></head>\n"
                        "<body bgcolor=\"white\">\n"
                        "<center><h1>500 Internal Error</h1></center>\n"
                        "<hr><center>CJJ WebServer/2.0 (Linux)</center>\n"
                        "</body>\n"
                        "</html>";

//可能在长连接的时候，parser自动机的类型会出现问题，待解决

HttpData::HttpData(EventLoop* loop, int connfd)
    :   loop_(loop),
        fd_(connfd),
        clientChannel_(new Channel(loop_, fd_)),
        inBuffer_(),
        outBuffer_(),
        checked_index_(0),
        error_(false),
        connectionState_(H_CONNECTED),
        parser_(HttpRequestParser::PARSE_REQUESTLINE),
        request_(),
        response_(),
        timer_(),
        isKeepAlive(false),
        latch_(1)
{
    clientChannel_->setReadHandler(std::bind(&HttpData::handleRead, this));
    clientChannel_->setWriteHandler(std::bind(&HttpData::handleWrite, this));
    clientChannel_->setConnHandler(std::bind(&HttpData::handleConn, this));
}

void HttpData::reset()
{
    this->resetParser();
    seperateTimer();
}

void HttpData::resetParser()
{
    checked_index_ = 0;
    parser_ = HttpRequestParser::PARSE_REQUESTLINE;
    request_.init();
    response_.init();
}

void HttpData::seperateTimer()
{
    std::shared_ptr<TimerNode> timer(timer_.lock());
    if(timer)
    {
        timer->clear();
        timer_.reset();
    }
}

void HttpData::newEvent()
{
    clientChannel_->setEvents(EPOLLIN|EPOLLET);
    loop_->addToPoller(clientChannel_, DEFAULT_TIMEOUT_SHORT);
}

void HttpData::handleClose()
{
    //将clientChannel从loop中取出来
    connectionState_ = H_DISCONNECTED;
    //由于不同的eventloop是不同的线程
    //这里防止在调用removeFromPoller的时候引起多线程之间的析构竞态条件
    //即，等待removeFromPoller调用结束后，再析构整个对象
    std::shared_ptr<HttpData> guard(shared_from_this());
    loop_->removeFromPoller(clientChannel_);
    latch_.countDown();
}

//读到的数据直接放在inbuffer的后面，即inbuffer+=str;
void HttpData::handleRead()
{
    __uint32_t& events_ = clientChannel_->getEvents();

    do
    {
        //读取数据
        bool zero = false;
        int read_num = readn(fd_, inBuffer_, zero);
        LOG<<"Request: "<<inBuffer_;
        //是否处于半关闭状态
        if(connectionState_ == H_DISCONNECTING)
        {
            inBuffer_.clear();
            break;//表示完成读操作
        }
        //读取返回值
        //出错，这里定义为客户端出错了
        if(read_num<0)
        {
           LOG<<"error: ret value < 0";
            error_ = true;
            handleError(BAD_REQUEST_PAGE, 400, "BAD REQUEST");
        }
        //出现了对端关闭的情况
        //关闭链接
        else if(zero)//这里表示read调用返回0，而不是由0个被写入了
        {
            LOG<<"Half Closing";
            connectionState_ = H_DISCONNECTING;
            if(read_num == 0)
            {
                break;//没有值，退出；如果有值，进行分析，因为还可以写
            }
        }
    } while (false);//进行数据读取
    //处理返回值
    HttpRequestParser::HTTP_CODE ret = HttpRequestParser::BAD_REQUEST;
    if(!error_)
    {
        // std::cout<<inBuffer_<<std::endl;
        //其他情况，正常返回，进行报文解析
        ret = HttpRequestParser::parse_content(inBuffer_, parser_, checked_index_, request_);
        switch(ret)
        {
            case HttpRequestParser::GET_REQUEST:
            {
                LOG<<"Get Request";
                fillResponse();
                break;
            }
            case HttpRequestParser::BAD_REQUEST:
            {
                LOG<<"Bad Request";
                error_ = true;
                handleError(BAD_REQUEST_PAGE, 400, "BAD REQUEST");
                break;
            }
            case HttpRequestParser::NO_REQUEST:
            {
                LOG<<"No Request";
                events_|=EPOLLIN;
                break;
            }
            case HttpRequestParser::INTERNAL_ERROR:
            {
                LOG<<"Internal Error";
                error_ = true;
                handleError(INTERNAL_ERROR_PAGE, 500, "INTERNAL ERROR");
                break;
            }
            default:
            {
                LOG<<"Internal Error";
                error_ = true;
                handleError(INTERNAL_ERROR_PAGE, 500, "INTERNAL ERROR");
                break;
            }
        }
    }
    if(!error_ && ret == HttpRequestParser::GET_REQUEST)
    {
        // this->reset();
        this->resetParser();
        if(!outBuffer_.empty())
        {
            handleWrite();
        }
    }
    else if(!error_ && ret == HttpRequestParser::NO_REQUEST)
    {
        // if(!inBuffer_.empty())
        // {
        //     handleRead();
        // }
        // else 
        events_ |= EPOLLIN;
    }
    //仍然有值，继续处理read；管线化
    else if(!error_ && ret != HttpRequestParser::NO_REQUEST)
    {
        // this->reset();
        this->resetParser();
    }
    // if(!error_ && !inBuffer_.empty())
    // {
    //     // this->reset();
    //     this->resetParser();
    //     // handleRead();
    //     events_ |= EPOLLIN;
    // }
    //继续读，长链接
    // if(!error_ && connectionState_ != H_DISCONNECTED)
    // {
    //     events_ |= EPOLLIN;
    // }

}

void HttpData::handleWrite()
{
    //如果出错的话，直接写了，所以没有出错的时候需要在这里写
    //并且需要连接不在关闭状态
    if(!error_ && connectionState_!=H_DISCONNECTED)
    {
        __uint32_t &events_ = clientChannel_->getEvents();
        if(writen(fd_, outBuffer_)<0)
        {
            LOG<<"error: writen failed";
            events_ = 0;
            error_ = true;//出错了
        }
        if(outBuffer_.size()>0)events_|=EPOLLOUT;
    }
}

void HttpData::handleConn()
{
    this->seperateTimer();//断开HttpData和timer的联系，因为HttpData的事件发生了
    __uint32_t& events_ = clientChannel_->getEvents();
    //没有出错、在连接中
    if(!error_ && connectionState_ == H_CONNECTED)
    {
        //有事件
        if(events_ != 0)
        { 
            int timeout = DEFAULT_TIMEOUT_SHORT;
            if(isKeepAlive)timeout = DEFAULT_TIMEOUT;
            //如果读和写共存，先写
            if((events_&EPOLLIN) && (events_&EPOLLOUT))
            {
                events_ = 0;
                events_ |= EPOLLOUT;
            }
            events_ |= EPOLLET;
            loop_->updateToPoller(clientChannel_, timeout);
        }
        //没有事件并且是长链接
        else if(isKeepAlive)
        {
            events_ |= (EPOLLIN | EPOLLET);
            loop_->updateToPoller(clientChannel_, DEFAULT_TIMEOUT);
        }
        //短链接，且没有事件注册，等待一小会儿
        else
        {
            events_ |=(EPOLLIN | EPOLLET);
            loop_->updateToPoller(clientChannel_, DEFAULT_TIMEOUT_SHORT);
            // //LOG<<"Do not Keep-Alive: close";
            // loop_->runInLoop(std::bind(&HttpData::handleClose, shared_from_this()));
        }
        isKeepAlive = false;
    }
    //处于半连接状态且有数据
    else if(!error_ && (connectionState_ == H_DISCONNECTING) && (events_ & EPOLLOUT))
    {
        if(outBuffer_.empty())
        {
            LOG<<"Half Closing No Output: close";
            loop_->runInLoop(std::bind(&HttpData::handleClose, shared_from_this()));
        }
        else
        {
            events_ = (EPOLLET | EPOLLOUT);
            //需要及时处理，否则关闭
            loop_->updateToPoller(clientChannel_, DEFAULT_TIMEOUT_SHORT);
        }
    }
    //其他情况，关闭链接
    else
    {
        LOG<<"error happened: close";
        loop_->runInLoop(std::bind(&HttpData::handleClose, shared_from_this()));
    }
}

void HttpData::handleError(char* msg, int status, std::string&& status_code)
{
    //遵守管线化、服务器端响应报文（依据请求报文顺序）先进先出
    //然而，这里出错了，所以先想办法发一下之前没发完的，然后直接发出错信息
    if(!outBuffer_.empty())writen(fd_, outBuffer_);

    std::string buff;
    buff += "HTTP/1.1 "+ std::to_string(status)+" "+status_code+"\r\n";
    buff += "Server:\tCJJ WebServer\r\n";
    buff += "Content-Length:\t"+std::to_string(strlen(msg))+"\r\n";
    buff += "Content-Type:\ttext/html\r\n";
    buff += "Connection:\tclose\r\n\r\n";
    buff += std::string(msg);

    // LOG<<"handleError: "<<buff;

    writen(fd_, buff);
}

void HttpData::fillResponse()
{
    //首先判断请求的页面是否存在、可以访问
    void* fileBody = nullptr;
    std::string filename("../pages");
    if(request_.mUri=="/")request_.mUri += "index.html";
    filename += request_.mUri;//得到类似 ../pages/index.html

    struct stat m_file_stat;
    //检查文件是否存在
    if(stat(filename.c_str(), &m_file_stat)<0)
    {
        error_ = true;
        handleError(NOT_FOUND_PAGE, 404, "NOT FOUND");
        return;
    }
    else//读取成功
    {
        //访问者是否有权限访问
        if(!(m_file_stat.st_mode & S_IROTH))
        {
            error_ = true;
            handleError(FORBIDDEN_PAGE, 403, "FORBIDDEN");
            return;
        }
        //可以访问，是否是文件名
        else if(S_ISDIR( m_file_stat.st_mode))
        {
            error_ = true;
            handleError(BAD_REQUEST_PAGE, 400, "BAD REQUEST");
            return;
        }
        //可以访问，打开文件描述符
        else
        {
            int file_fd = open(filename.c_str(), O_RDONLY);
            if(file_fd<0)
            {
                LOG<<"error: "<<filename<<" open failed";
                return;
            }
            fileBody = mmap(NULL, m_file_stat.st_size, PROT_READ, MAP_PRIVATE, file_fd, 0);
            close(file_fd);
            //共享内存打开失败
            if(fileBody == (void*)-1)
            {
                LOG<<"error: "<<filename<<" failed padding shared memory";
                error_ = true;
                handleError(INTERNAL_ERROR_PAGE, 500, "INTERNAL ERROR");
                return;
            }
        }

    }//文件操作结束

    //填充头部
    response_.setVersion(request_.mVersion);
    response_.setStatusCode(HttpRequestParser::FILE_REQUEST);
    isKeepAlive = request_.keep_alive;
    response_.setAlive(request_.keep_alive);
    response_.setHeaders("Server", "CJJ WebServer");
    response_.setHeaders("Content-Length", std::to_string(m_file_stat.st_size));
    // response_.setHeaders("Keep-Alive", "timeout="+std::to_string((request_.keep_alive?DEFAULT_TIMEOUT:DEFAULT_TIMEOUT_SHORT)));
    //注册MIME
    int dot_pos = filename.rfind('.');
    if(dot_pos==std::string::npos)
    {
        error_ = true;
        munmap(fileBody, m_file_stat.st_size);
        handleError(INTERNAL_ERROR_PAGE, 500, "INTERNAL ERROR");
        return;
    }
    else
    {
        response_.setMIME(filename.substr(dot_pos));
    }
    //得到报头
    outBuffer_ += response_.getResponseHeader();
    //Keep-Alive: timeout=x;
    //x的单位是s
    if(isKeepAlive) outBuffer_ += "Keep-Alive:\ttimeout="+std::to_string(DEFAULT_TIMEOUT/1000)+"\r\n";
    outBuffer_ += "\r\n";
        
    LOG<<outBuffer_;

    //读取共享内存数据
    outBuffer_ += std::string(static_cast<char*>(fileBody), m_file_stat.st_size);

    

    //别忘了注销共享内存
    munmap(fileBody, m_file_stat.st_size);
}