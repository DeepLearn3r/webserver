#pragma once
#include <sys/types.h>
#include <string>

ssize_t writen(int fd, void *buff, size_t n);
ssize_t writen(int fd, std::string& buf);
ssize_t readn(int fd, void *buff, size_t n);
ssize_t readn(int fd, std::string& buffer, bool& zero);//read函数是否有返回值0，如果有，那么表示客户关闭链接
ssize_t readn(int fd, std::string& buffer);

std::string& ltrim(std::string& s);
std::string& rtrim(std::string& s);
std::string& trim(std::string& s);

int setnonblocking(int fd);//设为非阻塞
void handleSigpipe();//处理sigpipe事件，即忽视
void setSockNoDelay(int fd);
int getListenSocket(int port);

