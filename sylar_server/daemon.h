//  守护进程
//父进程————>子进程出问题，重启子进程
//主(子)进程————>执行server服务器
#ifndef __SYLAR_DAEMON_H__
#define __SYLAR_DAEMON_H__

#include <unistd.h>
#include <functional>
#include "singLeton.h"
#include <memory>

namespace sylar
{

    /**
     * 进程生命执行周期
    */
    struct ProcessInfo
    {
        pid_t parent_id = 0;    //父进程id  (pid_t进程ID)
        pid_t main_id = 0;  //主(子)进程id
        uint64_t parent_start_time = 0; //父进程启动时间
        uint64_t main_start_time = 0;   //主(子)进程启动时间
        uint32_t restart_count = 0; //主(子)进程重启的次数

        std::string toString() const;   //返回进程生命周期字符串
    };

    typedef sylar::Singleton<ProcessInfo> ProcessInfoMgr;   //单例模式封装

    /**
     *  启动程序可以选择是否使用守护进程的方式
     *      argc 参数个数
     *      argv 参数值数组
     *      main_cb 启动函数(server)
     *      is_daemon 是否使用守护进程的方式
     * return：
     *      返回程序的执行结果
     */
    int start_daemon(int argc, char** argv
                    , std::function<int(int argc, char** argv)> main_cb
                    , bool is_daemon);

}

#endif
