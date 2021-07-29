#include "Server.h"

#include <memory>
#include <unistd.h>
#include <cstdlib>
#include <functional>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include "EventLoop.h"
#include "EventLoopThreadPool.h"
#include "Channel.h"
#include "HttpData.h"
#include "Utils.h"
#include "Logging.h"


Server::Server(EventLoop* loop, int threadNum, int port)
    :   loop_(loop),
        threadNum_(threadNum),
        threadPool_(new EventLoopThreadPool(loop, threadNum)),
        acceptChannel_(new Channel(loop)),
        port_(port),
        listenFd_(getListenSocket(port))
{
    if(listenFd_<0)
    {
        LOG<<"error: listen fd create failed";
        abort();
    }
    if(setnonblocking(listenFd_)<0)
    {
        close(listenFd_);
        LOG<<"error: listen fd set non-blocking failed";
        abort();
    }
    handleSigpipe();
    acceptChannel_->setFd(listenFd_);
    acceptChannel_->setConnHandler(std::bind(&Server::handleThisConn, this));
    acceptChannel_->setReadHandler(std::bind(&Server::handleNewConn, this));
}

void Server::start()
{
    threadPool_->start();
    acceptChannel_->setEvents(EPOLLIN|EPOLLET);
    loop_->addToPoller(acceptChannel_, 0);
}

void Server::handleNewConn()
{
    struct sockaddr_in client_address;
    memset(&client_address, 0, sizeof(client_address));
    socklen_t client_len = sizeof(client_address);
    int sockfd = 0;
    while((sockfd = accept(listenFd_, (struct sockaddr*)&client_address, &client_len))>0)
    {
        //轮询法分配eventloop
        EventLoop* slave_loop = threadPool_->getNextLoop();

        struct sockaddr_in peer_address;
        memset(&peer_address, 0, sizeof(peer_address));
        socklen_t peer_len = sizeof(peer_address);
        getpeername(sockfd, (struct sockaddr*)&peer_address, &peer_len);
        LOG<<"new connection from " << inet_ntoa(peer_address.sin_addr)
            <<":"<<ntohs(peer_address.sin_port);
        
        // //LOG<<"new connection from " << inet_ntoa(client_address.sin_addr)
        //     <<":"<<ntohs(client_address.sin_port);

        if(sockfd>=MAXFDS)
        {
            //超过最大文件描述符要求
            close(sockfd);
            LOG<<"warning: connection beyond MAXFDS";
            continue;
        }

        if(setnonblocking(sockfd)<0)
        {
            close(sockfd);
            LOG<<"warning: client fd set non-blocking failed";
            continue;
        }

        setSockNoDelay(sockfd);//关闭Nagle算法，防止粘包
        
        //构建HttpData，传slaveloop;
        std::shared_ptr<HttpData> clientData(new HttpData(slave_loop, sockfd));
        clientData->getChannel()->setHolder(clientData);
        slave_loop->queueInLoop(std::bind(&HttpData::newEvent, clientData));
    }
    acceptChannel_->setEvents(EPOLLIN | EPOLLET);
}

void Server::handleThisConn()
{
    loop_->updateToPoller(acceptChannel_, 0);
}


