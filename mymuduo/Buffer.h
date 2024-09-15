#pragma once

#include <vector>
#include <stdlib.h>
#include <string>
#include <algorithm>

/*                         Buffer
+-------------------+-------------------+-------------------+
| prependable bytes |  readable bytes   |   writable bytes  |
|                   |    (content)      |                   |
+-------------------+-------------------+-------------------+
|                   |                   |                   |
0      <==     ReaderIndex   <==   WriterIndex     <==     size          
*/


// 网络库底层的缓冲区类型定义
class Buffer
{
public:
    static const size_t kCheapPrepend = 8;
    static const size_t kInitialSize = 1024;

    explicit Buffer(size_t initial_size = kInitialSize)
        : buffer_(kCheapPrepend + initial_size)
        , ReaderIndex_(kCheapPrepend)
        , WriterIndex_(kCheapPrepend)
    {}

    size_t ReadableBytes() const { return WriterIndex_ - ReaderIndex_; }
    size_t WritableBytes() const { return buffer_.size() - WriterIndex_; }
    size_t PrependableBytes() const { return ReaderIndex_; }

    // 返回缓冲区中可读数据的起始地址
    const char* Peek() const { return Begin() + ReaderIndex_; }

    void Retrieve(size_t len)
    {
        if (len < ReadableBytes())
        {
            // 应用只读取了可读缓冲区数据的一部分, 还剩下ReaderIndex_ + len ==> WriterIndex_没读
            ReaderIndex_ += len; 
        }
        else // len == ReadableBytes()
        {
            RetrieveAll();
        }
    }

    void RetrieveAll()
    {
        ReaderIndex_ = kCheapPrepend;
        WriterIndex_ = kCheapPrepend;
    }

    // 把OnMessage函数上报的Buffer数据, 转化成string类型返回
    std::string RetrieveAllAsString()
    {
        return RetrieveAsString(ReadableBytes()); // 应用可读取数据的长度
    }

    std::string RetrieveAsString(size_t len)
    {
        std::string result(Peek(), len);
        Retrieve(len); // 上一句把缓冲区中可读数据读取完毕, 这里要对缓冲区进行复位操作
        return result;
    }

    // buffer_.size() - WriterIndex_  <==> len
    void EnsureWritableBytes(size_t len)
    {
        if (WritableBytes() < len)
        {
            MakeSpace(len); // 扩容函数
        } 
    }

    // 把[data, data+len]内存上的数据, 添加到Writable缓冲区中
    void Append(const char* data, size_t len)
    {
        EnsureWritableBytes(len);
        std::copy(data, data+len, BeginWrite());
        WriterIndex_ += len;
    }

    char* BeginWrite() { return Begin() + WriterIndex_; }
    const char* BeginWrite() const { return Begin() + WriterIndex_; }

    // 从fd上读取数据
    ssize_t ReadFd(int fd, int* saved_errno);
    // 通过fd发送数据
    ssize_t WriteFd(int fd, int* saved_errno);

private:
    char* Begin() { return &*buffer_.begin(); }
    const char* Begin() const { return &*buffer_.begin(); }

    void MakeSpace(size_t len)
    {
        if (WritableBytes() + PrependableBytes() < len + kCheapPrepend)
        {
            buffer_.resize(WriterIndex_ + len);
        }
        else
        {
            size_t readable = ReadableBytes();
            // 将可读数据向左移动, 使得WritableBytes() > len
            std::copy(Begin()+ReaderIndex_, Begin()+WriterIndex_, Begin()+kCheapPrepend);
            ReaderIndex_ = kCheapPrepend;
            WriterIndex_ = ReaderIndex_ + readable;
        }
    }

    std::vector<char> buffer_;
    size_t ReaderIndex_;
    size_t WriterIndex_;
};