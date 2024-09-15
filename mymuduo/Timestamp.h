#pragma once

#include <iostream>
#include <string>

// 时间类
class TimeStamp
{
public:
    TimeStamp();
    // 修饰只有一个参数的类构造函数, 以表明该构造函数是显式的, 而非隐式的
    // 多参构造函数本身就是显示调用的, 使用explit修饰没有较大意义
    // 禁止类对象之间的隐式转换, 以及禁止隐式调用拷贝构造函数
    explicit TimeStamp(int64_t MicroSecondsSinceEpoch);
    static TimeStamp now();
    std::string ToString() const;

private:
    int64_t MicroSecondsSinceEpoch_;
};