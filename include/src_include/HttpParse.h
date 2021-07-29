#pragma once

#include "HttpRequest.h"
#include <string>

//用于解析Http报文
class HttpRequestParser
{
public:
    enum PARSE_STATE
    {
        PARSE_REQUESTLINE = 0,//正在分析请求行
        PARSE_HEADER,//正在分析头部字段
        PARSE_BODY//正在分析内容实体
    };
    enum LINE_STATE
    {
        LINE_OK = 0,//完整行
        LINE_BAD,//行出错
        LINE_OPEN//行数据不完整
    };
    enum HTTP_CODE
    {
        NO_REQUEST = 0,//请求不完整
        GET_REQUEST,//获得一个完整的请求
        BAD_REQUEST,//请求有语法错误
        INTERNAL_ERROR,//服务器内部有问题
        CLOSED_CONNECTION,//客户端关闭链接
        FORBIDDEN_REQUEST,//资源没有权限
        FILE_REQUEST,//可获得的资源
        NO_RESOURCE//没有这个文件
    };

    //入口函数
    static HTTP_CODE parse_content(std::string& buf, PARSE_STATE&, int& checked_index, HttpRequest&);

    static LINE_STATE parse_line(std::string& buf, int& checked_index, int& read_index, std::string& line);

    static HTTP_CODE parse_requestline(std::string& buf, PARSE_STATE&, HttpRequest&);

    static HTTP_CODE parse_header(std::string& buf,  PARSE_STATE&, HttpRequest&);

    static HTTP_CODE parse_body(std::string& buf, HttpRequest&);
};