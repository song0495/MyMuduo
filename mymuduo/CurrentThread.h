#pragma once

#include <unistd.h>
#include <sys/syscall.h>

namespace CurrentThread
{
    extern __thread int t_CacheTid;

    void CacheTid();

    inline int Tid()
    {
        if (__builtin_expect(t_CacheTid == 0, 0))
        {
            CacheTid();
        }
        return t_CacheTid;
    }
}