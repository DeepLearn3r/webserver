// @Author Lin Ya
// @Email xxbbb@vip.qq.com
#pragma once
#include <string>
#include "LogStream.h"


/*
内部类Impl：用于存储时间、调用文件名、调用代码所在行数、一个LogStream对象
*/
class Logger {
 public:
  Logger(const char *basename, int line);
  ~Logger();
  LogStream &stream() { return impl_.stream_; }

  static void setLogFileName(std::string fileName) { logFileName_ = fileName; }
  static std::string getLogFileName() { return logFileName_; }

 private:
  class Impl {
   public:
    Impl(const char *basename, int line);
    void formatTime();

    LogStream stream_;
    int line_;
    std::string basename_;
  };
  Impl impl_;
  static std::string logFileName_;
};

#define LOG Logger(__FILE__, __LINE__).stream()