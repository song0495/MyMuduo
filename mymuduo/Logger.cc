#include <iostream>

#include "Logger.h"
#include "Timestamp.h"

// 获取日志单例
Logger& Logger::Instance()
{
    static Logger logger;
    return logger;
}

// 设置日志级别
void Logger::SetLogLevel(int level)
{
    LogLevel_ = level;
}

// 写日志
void Logger::Log(std::string msg)
{
    switch (LogLevel_)
    {
    case INFO:
        std::cout << "[INFO]";
        break;
    case ERROR:
        std::cout << "[ERROR]";
        break;
    case FATAL:
        std::cout << "[FATAL]";
        break;
    case DEBUG:
        std::cout << "[DEBUG]";
        break;
    default:
        break;
    }

    // 打印时间和msg
    std::cout << TimeStamp::now().ToString() << " ==> " << msg << std::endl;
}