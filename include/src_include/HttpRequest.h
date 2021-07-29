#pragma once

#include <string>
#include <unordered_map>

/*
*   HttpRequest
*   记录Http请求报文的头部信息
*   主要记录：
*   方法、URL、HTTP版本和其他头部字段
*/
struct HttpRequest
{
    enum HTTP_VERSION
    {
        HTTP_1_0=0,
        HTTP_1_1,
        VERSION_NOT_SUPPORT
    };
    enum HTTP_METHOD
    {
        GET=0,
        HEAD,
        METHOD_NOT_SUPPORT
    };
    enum HTTP_HEADERS
    {
        Host = 0,//请求的主机名和端口号
        User_Agent,//用户代理，用什么工具发出的请求
        Connection,//请求报文请求建立什么样的连接
        Accept_Encoding,//支持使用的编码方式
        Accept_Language,//支持使用的语言
        Accept,//服务器能发送的媒体类型
        Cache_Control,//缓存指标,
        Content_Length,
        Upgrade_Insecure_Requests
    };

    template<typename T>
    struct EnumHash
    {
        std::size_t operator()(T t) const
        {
            return static_cast<std::size_t>(t);
        }
    };

    HttpRequest()
    {
        init();
    }

    void init();

    static std::unordered_map<std::string, HTTP_HEADERS> header_map;//记录英文短语 和 HTTP头部字段在enum中的一一对应关系

    HTTP_VERSION mVersion;
    HTTP_METHOD mMethod;
    
    std::string mUri;

    std::string mContent;//body

    bool keep_alive;

    std::unordered_map<HTTP_HEADERS, std::string, EnumHash<HTTP_HEADERS>> mHeaders;//记录HTTP请求报文中对应的字段

};