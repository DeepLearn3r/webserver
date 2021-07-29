#include "Utils.h"

#include <string>
#include <cstring>
#include <unistd.h>
#include <errno.h>
#include <cstdlib>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <signal.h>
#include <fcntl.h>
#include <assert.h>
#include "Logging.h"


const int MAX_BUFF = 4096;


ssize_t writen(int fd, void* buff, size_t n)
{
    size_t nleft = n;
    ssize_t nwritten = 0;
    ssize_t writeSum = 0;
    char* ptr = (char*)buff;
    while(nleft>0)
    {
        //出错处理
        if((nwritten = write(fd, ptr, nleft))<0)
        {
            if(errno == EINTR)//被信号打断了，再来
            {
                nwritten = 0;
                continue;
            }else if(errno == EAGAIN)//事件尚未发生，表示缓冲区没有数据
            {
                return writeSum;
            }
            else return -1;
        }
        writeSum += nwritten;
        nleft-=nwritten;
        ptr+=nwritten;
    }
    return writeSum;
}

ssize_t writen(int fd, std::string& buf)
{
    size_t nleft = buf.length();
    ssize_t nwritten = 0;
    ssize_t writeSum = 0;
    const char* ptr = buf.c_str();
    while(nleft>0)
    {
        if((nwritten=write(fd, ptr, nleft))<0)
        {
            if(errno == EINTR)
            {
                nwritten = 0;
                continue;
            }
            else if(errno == EAGAIN)
            {
                nwritten = 0;
                break;
            }
            else return -1;
        }
        nleft -= nwritten;
        writeSum += nwritten;
        ptr += nwritten;
    }
    if(writeSum==buf.length())buf.clear();
    else buf = buf.substr(writeSum);
    return writeSum;
}

ssize_t readn(int fd, void* buff, size_t n)
{
    size_t nleft = n;
    ssize_t nread = 0;
    ssize_t readSum = 0;
    char* ptr = (char*)buff;
    while(nleft>0)
    {
        if((nread = read(fd, ptr, nleft))<0)
        {
            if(errno == EAGAIN)
            {
                return readSum;
            }
            else if(errno == EINTR)
            {
                nread = 0;
                continue;
            }
            else 
            {
                perror("read error");
                return -1;
            }
        }
        else if(nread == 0)return readSum;
        readSum += nread;
        nleft -= nread;
        ptr += nread;
    }
    return readSum;
}

ssize_t readn(int fd, std::string& buffer)
{
    ssize_t nread = 0;
    ssize_t readSum = 0;
    while(true)
    {
        char buf[MAX_BUFF];
        if((nread=read(fd, buf, MAX_BUFF))<0)
        {
            if(errno == EINTR)continue;
            else if(errno == EAGAIN) return readSum;
            else
            {
                perror("read error");
                return -1;
            }
        }
        else if(nread == 0) break;//对端关闭连接
        
        readSum += nread;
        buffer += std::string(buf, buf+nread);
    }
    return readSum;
}

ssize_t readn(int fd, std::string& buffer, bool& zero)
{
    ssize_t nread = 0;
    ssize_t readSum = 0;
    while(true)
    {
        char buf[MAX_BUFF];
        if((nread=read(fd, buf, MAX_BUFF))<0)
        {
            if(errno == EINTR)continue;
            else if(errno == EAGAIN) return readSum;
            else
            {
                perror("read error");
                return -1;
            }
        }
        else if(nread == 0) 
        {
            zero = true;
            break;
        }
        
        readSum += nread;
        buffer += std::string(buf, buf+nread);
    }
    return readSum;
}



std::string& ltrim(std::string& s)
{
    if(s.empty())return s;
    s.erase(0, s.find_first_not_of(" \t"));
    return s;
}

std::string& rtrim(std::string& s)
{
    if(s.empty())return s;
    int pos = s.find_last_not_of(" \t");
    s.erase(pos+1, s.length()-pos-1);
    return s;
}

std::string& trim(std::string& s)
{
    return ltrim(rtrim(s));
}

int setnonblocking(int fd)
{
    int old = fcntl(fd, F_GETFL);
    if(old<0)return -1;
    int new_option = old | O_NONBLOCK;
    if(fcntl(fd, F_SETFL, new_option)<0)return -1;
    return old;
}

void handleSigpipe()
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = SIG_IGN;
    sa.sa_flags |= SA_RESTART;
    sigfillset(&sa.sa_mask);
    assert(sigaction(SIGPIPE, &sa, NULL)!=-1);
}

void setSockNoDelay(int fd)
{
    int no_delay = 1;
    //TCP_NODELAY在头文件<netinet/tcp.h>下声明并定义
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &no_delay, sizeof(no_delay));
}

int getListenSocket(int port)
{
    if(port<0 || port>65535)return -1;

    int listen_fd = socket(PF_INET, SOCK_STREAM, 0);
    if(listen_fd<0)return -1;

    //消除address already in use错误
    int enable = 1;
    if(setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable))<0)
    {
        close(listen_fd);
        return -1;
    }

    int ret = 0;
    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_port = htons(port);
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    ret = bind(listen_fd, (struct sockaddr*)&address, sizeof(address));
    if(ret<0)
    {
        close(listen_fd);
        return -1;
    }

    //注意，最大监听队列长度为listenq，可以查，也可以盖
    ret = listen(listen_fd, 2048);
    if(ret<0)
    {
        close(listen_fd);
        return -1;
    }

    return listen_fd;

}



