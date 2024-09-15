#pragma once

/*
noncopyable被继承以后, 派生类对象可以正常构造和析构, 但无法进行拷贝构造和赋值操作
因为如果派生类对象想要进行拷贝构造或赋值操作, 需要先调用基类的拷贝构造或赋值操作

tips:   当派生类定义了拷贝或移动构造函数, 必须显式地调用基类的拷贝或移动构造函数, 
        否则会调用基类的默认构造函数, 也就失去了拷贝构造的意义
*/

class noncopyable
{
public:
    noncopyable(const noncopyable&) = delete;
    noncopyable& operator=(const noncopyable&) = delete;

protected:
    noncopyable() = default;
    ~noncopyable() = default;
};