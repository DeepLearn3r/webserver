#include "HttpResponse.h"
#include "HttpParse.h"
// #include <cstring>
#include <string>
#include <unordered_map>


std::unordered_map<std::string, std::string> HttpResponse::mime_map = 
{
    {".html", "text/html"},
    {".xml", "text/xml"},
    {".xhtml", "application/xhtml+xml"},
    {".txt", "text/plain"},
    {".rtf", "application/rtf"},
    {".pdf", "application/pdf"},
    {".word", "application/msword"},
    {".png", "image/png"},
    {".gif", "image/gif"},
    {".jpg", "image/jpeg"},
    {".jpeg", "image/jpeg"},
    {".au", "audio/basic"},
    {".mpeg", "video/mpeg"},
    {".mpg", "video/mpeg"},
    {".avi", "video/x-msvideo"},
    {".gz", "application/x-gzip"},
    {".tar", "application/x-tar"},
    {".css", "text/css"},
    {"", "text/plain"},
    // {"default","text/plain"}
    {"default", "text/html"}
};




void HttpResponse::setStatusCode(HttpRequestParser::HTTP_CODE http_code)
{
    switch(http_code)
    {
        case HttpRequestParser::FILE_REQUEST:
        {
            m_status_code = HttpStatusCode::m200_OK;
            m_description = "OK";
            break;
        }
        case HttpRequestParser::BAD_REQUEST:
        {
            m_status_code = HttpStatusCode::m400_BAD_REQUEST;
            m_description = "Bad Request";
            break;
        }
        case HttpRequestParser::FORBIDDEN_REQUEST:
        {
            m_status_code = HttpStatusCode::m403_FORBIDDEN;
            m_description = "Forbidden";
            break;
        }
        case HttpRequestParser::NO_RESOURCE:
        {
            m_status_code = HttpStatusCode::m404_NOT_FOUND;
            m_description = "Not Found";
            break;
        }
        case HttpRequestParser::INTERNAL_ERROR:
        {
            m_status_code = HttpStatusCode::m500_INTERNAL_ERROR;
            m_description = "Internal Error";
            break;
        }
        default:
        {
            m_status_code = HttpStatusCode::m500_INTERNAL_ERROR;
            m_description = "Internal Error";
            break;
        }
    }
}


std::string HttpResponse::getStatusDescription()
{
    return m_description;
}


void HttpResponse::setVersion(HttpRequest::HTTP_VERSION version)
{
    m_version = version;
}


void HttpResponse::setMIME(std::string posix)
{
    if(mime_map.find(posix)==mime_map.end())
    {
        m_mime = mime_map["default"];
    }
    else 
    {
        m_mime = mime_map[posix];
    }
}


void HttpResponse::setHeaders(const std::string& key, const std::string& value)
{
    mHeaders[key] = value;
}


void HttpResponse::setAlive(bool ka)
{
    keep_alive = ka;//加入连接状态
    // keep_alive = false;//先拿无状态连接试试手
}


std::string HttpResponse::getResponseHeader()
{
    std::string rheader("");
    if(m_version == HttpRequest::HTTP_1_1)
    {
        rheader+="HTTP/1.1 ";
    }
    else rheader+="HTTP/1.0 ";

    rheader += std::to_string((int)m_status_code) + " " + m_description + "\r\n";

    for(auto it = mHeaders.begin(); it!=mHeaders.end(); ++it)
    {
        rheader += it->first + ":\t" + it->second + "\r\n";
    }

    if(keep_alive)
    {
        rheader += "Connection:\tkeep-alive\r\n";
    }
    else
    {
        rheader += "Connection:\tclose\r\n";
    }

    rheader+="Content-Type:\t"+m_mime+"\r\n";

    return rheader;
}

void HttpResponse::init()
{
    m_status_code=m500_INTERNAL_ERROR;
    m_description="Internal Error";
    m_version=HttpRequest::HTTP_1_1;
    m_mime="text/html";
    keep_alive=false;
    mHeaders.clear();
}