#include <errno.h>
#include <sys/uio.h>
#include <unistd.h>

#include "Buffer.h"

// 从fd上读取数据 Poller工作在LT模式
// Buffer缓冲区是有大小的, 但是从fd上读数据的时候, 却不知道tcp数据最终的大小
ssize_t Buffer::ReadFd(int fd, int* saved_errno)
{
    char extrabuf[65536] = {0}; // 栈上的内存空间 64k
    iovec vec[2];
    const size_t writable = WritableBytes(); // Buffer底层缓冲区剩余可写空间大小

    vec[0].iov_base = Begin() + WriterIndex_;
    vec[0].iov_len = writable;

    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof(extrabuf);

    const int iovcnt = (writable <sizeof(extrabuf)) ? 2 : 1;
    const ssize_t n = readv(fd, vec, iovcnt);
    if (n < 0)
    {
        *saved_errno = errno;
    }
    else if(n < writable) // Buffer的可写缓冲区已经够存储读出来的数据了
    {
        WriterIndex_ += n;
    }
    else // extrabuf里面也写入了数据
    {
        WriterIndex_ = buffer_.size();
        Append(extrabuf, n - writable); // WriterIndex_开始写(n - writable)大小的数据
    }

    return n;
}

ssize_t Buffer::WriteFd(int fd, int* saved_errno)
{
    ssize_t n = write(fd, Peek(), ReadableBytes());
    if (n < 0)
    {
        *saved_errno = errno;
    }
    
    return n;
}