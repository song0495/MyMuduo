#pragma once

#include <string>

#include "noncopyable.h"

#define LOG_INFO(logmsgformat, ...)                         \
    do                                                      \
    {                                                       \
        Logger& logger = Logger::Instance();                \
        logger.SetLogLevel(INFO);                           \
        char buf[BUFSIZ] = {0};                             \
        snprintf(buf, BUFSIZ, logmsgformat, ##__VA_ARGS__); \
        logger.Log(buf);                                    \
    } while (0)
    
#define LOG_ERROR(logmsgformat, ...)                        \
    do                                                      \
    {                                                       \
        Logger& logger = Logger::Instance();                \
        logger.SetLogLevel(ERROR);                          \
        char buf[BUFSIZ] = {0};                             \
        snprintf(buf, BUFSIZ, logmsgformat, ##__VA_ARGS__); \
        logger.Log(buf);                                    \
    } while (0)

#define LOG_FATAL(logmsgformat, ...)                        \
    do                                                      \
    {                                                       \
        Logger& logger = Logger::Instance();                \
        logger.SetLogLevel(FATAL);                          \
        char buf[BUFSIZ] = {0};                             \
        snprintf(buf, BUFSIZ, logmsgformat, ##__VA_ARGS__); \
        logger.Log(buf);                                    \
        exit(-1);                                           \
    } while (0)

#ifdef MUDEBUG
#define LOG_DEBUG(logmsgformat, ...)                        \
    do                                                      \
    {                                                       \
        Logger& logger = Logger::Instance();                \
        logger.SetLogLevel(DEBUG);                          \
        char buf[BUFSIZ] = {0};                             \
        snprintf(buf, BUFSIZ, logmsgformat, ##__VA_ARGS__); \
        logger.Log(buf);                                    \
    } while (0)
#else
#define LOG_DEBUG(logmsgformat, ...) 
#endif


// 定义日志的级别 INFO ERROR FATAL DEBUG
enum LogLevel
{
    INFO, // 普通消息
    ERROR, // 错误信息
    FATAL, // core信息
    DEBUG, // 调试信息
};

// 输出一个日志类
class Logger: noncopyable // 默认私有继承
{
public:
    // 获取日志单例
    static Logger& Instance();
    // 设置日志级别
    void SetLogLevel(int level);
    // 写日志
    void Log(std::string msg);

private:
    int LogLevel_;
    Logger(){}
};