#include "CurrentThread.h"

namespace CurrentThread
{
    __thread int t_CacheTid = 0;

    void CacheTid()
    {
        if (t_CacheTid == 0)
        {
            // 通过linux系统调用, 获取当前线程的tid值
            t_CacheTid = static_cast<pid_t>(syscall(SYS_gettid));
        }
    }
}