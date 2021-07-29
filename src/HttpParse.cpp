#include "HttpParse.h"
#include "HttpRequest.h"
#include "Utils.h"
#include <cstring>
#include <string>
#include <algorithm>
#include <cctype>
#include <cstdlib>

#include <iostream>

#define CR '\r'
#define LF '\n'
#define LINE_END '\0'
#define PASS


//是否是完整的一行
//直接作用在buf上，如果有完整的一行，割给line
HttpRequestParser::LINE_STATE HttpRequestParser::parse_line(std::string& buf, int& checked_index, int& read_index, std::string& line)
{
    char temp;
    for(;checked_index<read_index;++checked_index)
    {
        temp = buf[checked_index];
        if(temp==CR)
        {
            if(checked_index+1 == read_index)return LINE_OPEN;
            else if(buf[checked_index+1]==LF)
            {
                // buf[checked_index++] = LINE_END;
                // buf[checked_index++] = LINE_END;
                line.clear();
                line = buf.substr(0, checked_index);
                checked_index+=2;
                buf = buf.substr(checked_index);
                checked_index = 0;
                read_index = buf.length();
                return LINE_OK;
            }
            else return LINE_BAD;
        }
        if(temp == LF)
        {
            if(checked_index>1 && buf[checked_index-1]==CR)
            {
                // buf[checked_index-1]==LINE_END;
                // buf[checked_index++]==LINE_END;
                line.clear();
                line = buf.substr(0, checked_index-1);
                checked_index++;
                buf = buf.substr(checked_index);
                checked_index = 0;
                read_index = buf.length();
                return LINE_OK;
            }
            return LINE_BAD;
        }
    }
    return LINE_OPEN;
}



//该行是请求行
HttpRequestParser::HTTP_CODE HttpRequestParser::parse_requestline(std::string& buf, PARSE_STATE& parse, HttpRequest& request)
{
    char* temp = const_cast<char*>(buf.c_str());
    char* url = strpbrk(temp, " \t");
    if(!url)return BAD_REQUEST;

    *url++ = LINE_END;
    char* method = temp;
    if(strcasecmp(method, "GET")==0)
    {
        request.mMethod = HttpRequest::GET;
    }
    else if(strcasecmp(method, "HEAD")==0)
    {
        request.mMethod = HttpRequest::HEAD;
    }
    else{ request.mMethod = HttpRequest::METHOD_NOT_SUPPORT; return BAD_REQUEST;}

    url+=strspn(url, " \t");//消除头部空白
    char* version = strpbrk(url, " \t");//定位version
    if(!version)return BAD_REQUEST;
    *version++ = LINE_END;
    version+=strspn(version, " \t");//消除头部空白

    //检查url正确性
    if(strncasecmp(url, "http://", 7)==0)
    {
        url+=7;
        url = strchr(url, '/');
    }
    if(!url || *url!='/')return BAD_REQUEST;
    request.mUri = std::string(url);

    //进一步处理mUri，使诸如/index.html?8这种的变为/index.html
    int dot_pos = request.mUri.rfind('.');
    if(dot_pos!=std::string::npos)
    {
        int ques_pos = request.mUri.find('?', dot_pos+1);
        if(ques_pos!=std::string::npos)
        {
            request.mUri = request.mUri.substr(0, ques_pos);
        }
    }

    //version
    if(strncasecmp(version, "HTTP/1.0", 8)==0)
    {
        request.mVersion = HttpRequest::HTTP_1_0;
    }
    else if(strncasecmp(version, "HTTP/1.1", 8)==0)
    {
        request.mVersion = HttpRequest::HTTP_1_1;
    }
    else
    {
        request.mVersion = HttpRequest::VERSION_NOT_SUPPORT;
        return BAD_REQUEST;
    }

    parse = PARSE_HEADER;
    return NO_REQUEST;
}


//解析头部字段
HttpRequestParser::HTTP_CODE HttpRequestParser::parse_header(std::string& buf, PARSE_STATE& parse, HttpRequest& request)
{
    if(buf.empty())
    {
        // if(request.mMethod == HttpRequest::GET || request.mMethod == HttpRequest::HEAD)
        // {
        //     return GET_REQUEST;
        // }

        // parse = PARSE_BODY;
        // return NO_REQUEST;

        if(request.mHeaders.find(HttpRequest::Content_Length)!=request.mHeaders.end() && request.mHeaders[HttpRequest::Content_Length]!=std::string("0"))
        {
            parse = PARSE_BODY;
            return NO_REQUEST;
        }
        return GET_REQUEST;
    }
    char* temp = const_cast<char*>(buf.c_str());
    char* value = strchr(temp, ':');
    *value++ = LINE_END;
    value += strspn(value, "\t");
    
    std::string key_(temp);
    std::string value_(value);

    trim(key_);
    trim(value_);

    std::transform(key_.begin(), key_.end(), key_.begin(), toupper);
    //here

    if(key_ == std::string("CONNECTION") && value_ == std::string("keep-alive"))
    {
        request.keep_alive = true;
    }


    if(HttpRequest::header_map.find(key_)!=HttpRequest::header_map.end())
        request.mHeaders.insert(std::pair<HttpRequest::HTTP_HEADERS, std::string>(HttpRequest::header_map[key_], value_));
    
    return NO_REQUEST;
}

HttpRequestParser::HTTP_CODE HttpRequestParser::parse_body(std::string& buf, HttpRequest& request)
{
    if(request.mHeaders.find(HttpRequest::Content_Length)==request.mHeaders.end())
    {
        return BAD_REQUEST;
    }
    request.mContent.clear();
    // request.mContent = buf;
    long content_length = atol(request.mHeaders[HttpRequest::Content_Length].c_str());

    // using std::cout;
    // using std::endl;

    // cout<<request.mContent.length()<<endl;
    // cout<<content_length<<endl;

    // if(request.mContent.length()==content_length)
    // {
    //     return GET_REQUEST;//由于长度和读取的字符串长度对不上，请求失败
    // }
    // else if(request.mContent.length()> content_length)
    // {
    //     return BAD_REQUEST;
    // }
    if(buf.length() > content_length)
    {
        request.mContent = buf.substr(0, content_length);
        buf = buf.substr(content_length);
        return GET_REQUEST;
    }
    else if(buf.length() == content_length)
    {
        request.mContent = buf;
        buf.clear();
        return GET_REQUEST;
    }

    return NO_REQUEST;
}


/*
params:
buffer: 读取缓冲区
checked_index:当前已经分析完了多少字节的客户数据
parse_state:解析状态
read_index:当前已经读了多少字节的客户信息
start_line:行在buffer中的起始位置

description:
解析传来的数据，判断是否是一个完整的行
如果不是一个完整的行，则是行出错、行数据不完整，返回相应HTTP_CODE
如果是一个完整的行，
则分别进行请求行解析、头部解析以及头部实体解析

*/
HttpRequestParser::HTTP_CODE HttpRequestParser::parse_content
                                                                        (
                                                                        std::string& buf, 
                                                                        PARSE_STATE& parse,
                                                                        int& checked_index, 
                                                                        HttpRequest& request
                                                                        )
{
    LINE_STATE linestate = LINE_OK;
    std::string line;
    int read_index = buf.length();
    //如果parse是解析实体字段，并且前面的头部解析没有出错，那么直接进入parse_body函数
    while((parse==PARSE_BODY &&(linestate==LINE_OK))
        ||((linestate=parse_line(buf, checked_index, read_index, line))==LINE_OK))
    {
        // char* temp = buf+start_line;
        // start_line = checked_index;
        // std::cout<<line<<std::endl;

        HTTP_CODE code = NO_REQUEST;
        switch(parse)
        {
            case PARSE_REQUESTLINE:
            {
                code = parse_requestline(line, parse, request);
                if(code == BAD_REQUEST)return BAD_REQUEST;
                break;
            }
            case PARSE_HEADER:
            {
                code = parse_header(line, parse, request);
                if(code == GET_REQUEST)return GET_REQUEST;
                break;
            }
            case PARSE_BODY:
            {
                if(buf.empty()) return NO_REQUEST;

                code = parse_body(buf, request);
                if(code == GET_REQUEST)return GET_REQUEST;
                else if(code==BAD_REQUEST)return BAD_REQUEST;
                else return NO_REQUEST;
                // if(code==NO_REQUEST)std::cout<<"PARSE_BODY NO REQUEST"<<std::endl;
                // linestate = LINE_OPEN;
                break;
            }
            default:
            {
                return INTERNAL_ERROR;
            }
        }
    }
    if(linestate == LINE_OPEN)return NO_REQUEST;
    else return BAD_REQUEST;
}


