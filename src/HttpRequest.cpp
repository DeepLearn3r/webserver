#include "HttpRequest.h"

std::unordered_map<std::string, HttpRequest::HTTP_HEADERS> HttpRequest::header_map = 
{
    {"HOST",                      HttpRequest::Host},
    {"USER-AGENT",                HttpRequest::User_Agent},
    {"CONNECTION",                HttpRequest::Connection},
    {"ACCEPT-ENCODING",           HttpRequest::Accept_Encoding},
    {"ACCEPT-LANGUAGE",           HttpRequest::Accept_Language},
    {"ACCEPT",                    HttpRequest::Accept},
    {"CACHE-CONTROL",             HttpRequest::Cache_Control},
    {"CONTENT-LENGTH",          HttpRequest::Content_Length},
    {"UPGRADE-INSECURE-REQUESTS", HttpRequest::Upgrade_Insecure_Requests}
};

void HttpRequest::init()
{
    mVersion = VERSION_NOT_SUPPORT;
    mMethod = METHOD_NOT_SUPPORT;
    mUri = "";
    mContent = "";
    mHeaders.clear();
    keep_alive = false;
}