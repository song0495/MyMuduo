#include <time.h>

#include "Timestamp.h"

TimeStamp::TimeStamp(): MicroSecondsSinceEpoch_(0) {}

TimeStamp::TimeStamp(int64_t MicroSecondsSinceEpoch) 
    : MicroSecondsSinceEpoch_(MicroSecondsSinceEpoch) 
{}

TimeStamp TimeStamp::now()
{
    return TimeStamp(time(NULL));
}

std::string TimeStamp::ToString() const
{
    char buf[128] = {0};
    tm* tm_time = localtime(&MicroSecondsSinceEpoch_);
    snprintf(buf, 128, "%4d/%02d/%02d %02d:%02d:%02d", // 用'0'填充
    tm_time->tm_year + 1900, 
    tm_time->tm_mon + 1, 
    tm_time->tm_mday, 
    tm_time->tm_hour, 
    tm_time->tm_min, 
    tm_time->tm_sec);

    return buf;
}