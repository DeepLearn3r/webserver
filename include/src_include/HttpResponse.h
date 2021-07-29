#pragma once

#include "HttpParse.h"
#include "HttpRequest.h"
#include <string>
#include <unordered_map>

class HttpResponse
{
public:
    enum HttpStatusCode
    {
        m200_OK = 200,
        m400_BAD_REQUEST = 400,
        m403_FORBIDDEN = 403,
        m404_NOT_FOUND = 404,
        m500_INTERNAL_ERROR =500
    };

    static std::unordered_map<std::string, std::string> mime_map;

public:
    HttpResponse()
    {
        init();
    }

    void init();

    void setStatusCode(HttpRequestParser::HTTP_CODE);

    std::string getStatusDescription();

    void setMIME(std::string);

    void setVersion(HttpRequest::HTTP_VERSION);

    void setHeaders(const std::string& key, const std::string& value);

    void setAlive(bool);

    std::string getResponseHeader();

    bool isAlive() const {return keep_alive;}


private:
    HttpStatusCode m_status_code;//状态码

    std::string m_description;//状态码描述

    HttpRequest::HTTP_VERSION m_version;//HTTP版本

    std::string m_mime;//多用途互联网邮件拓展类型

    bool keep_alive;//是否是长链接

    std::unordered_map<std::string, std::string> mHeaders;//报文首部字段


};