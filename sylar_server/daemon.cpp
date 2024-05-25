#include "daemon.h"
#include "log.h"
#include "config.h"
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

namespace sylar
{

    static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

    //重启时间间隔
    static sylar::ConfigVar<uint32_t>::ptr g_daemon_restart_interval
        = sylar::Config::Lookup("daemon.restart_interval", (uint32_t)5, "daemon restart interval");

    std::string ProcessInfo::toString() const   //返回进程生命周期字符串
    {
        std::stringstream ss;
        ss << "[ProcessInfo parent_id=" << parent_id
        << " main_id=" << main_id
        << " parent_start_time=" << sylar::Time2Str(parent_start_time)
        << " main_start_time=" << sylar::Time2Str(main_start_time)
        << " restart_count=" << restart_count << "]";
        return ss.str();
    }

    static int real_start(int argc, char** argv,    //执行主(子)进程的执行函数
                        std::function<int(int argc, char** argv)> main_cb)
    {
        ProcessInfoMgr::GetInstance()->main_id = getpid();  //主(子)进程id  getpid()返回当前进程ID
        ProcessInfoMgr::GetInstance()->main_start_time = time(0);   //主(子)进程启动时间
        return main_cb(argc, argv); //执行server函数
    }

    static int real_daemon(int argc, char** argv,   //在父进程中，负责创建子进程和重启子进程
                        std::function<int(int argc, char** argv)> main_cb)
    {
        daemon(1, 0);   //守护进程  1 要不要上下文指向根路径    0 要不要关闭标准的输入输出
        ProcessInfoMgr::GetInstance()->parent_id = getpid();    //父进程id  getpid()返回当前进程ID
        ProcessInfoMgr::GetInstance()->parent_start_time = time(0); //父进程启动时间
        while(true)
        {
            pid_t pid = fork(); //fork生成一个进程      调用fork函数的进程为父进程，新生成的进程为子进程。
                                //1）在父进程中，fork返回新创建子进程的进程ID；
                                //2）在子进程中，fork返回0；
                                //3）如果出现错误，fork返回一个负值；
            if(pid == 0)    //在子进程中
            {
                //子进程中返回
                ProcessInfoMgr::GetInstance()->main_id = getpid();  //主(子)进程id  getpid()返回当前进程ID
                ProcessInfoMgr::GetInstance()->main_start_time  = time(0);  //主(子)进程启动时间
                SYLAR_LOG_INFO(g_logger) << "process start pid=" << getpid();   //
                return real_start(argc, argv, main_cb);
            }
            else if(pid < 0)    //失败
            {
                SYLAR_LOG_ERROR(g_logger) << "fork fail return=" << pid
                    << " errno=" << errno << " errstr=" << strerror(errno);
                return -1;
            }
            else    //在父进程中
            {
                //父进程中返回
                int status = 0;
                waitpid(pid, &status, 0);   //当指定等待的子进程已经停止运行或结束了，则waitpid()会立即返回；
                                            //但是如果子进程还没有停止运行或结束，则调用waitpid()函数的父进程则会被阻塞，暂停运行。
                                            //  status 保存子进程的状态信息
                if(status)  //子进程出问题
                {
                    if(status == 9) //结束
                    {
                        SYLAR_LOG_INFO(g_logger) << "killed";
                        break;
                    }
                    else    //停止运行(重启)
                    {
                        SYLAR_LOG_ERROR(g_logger) << "child crash pid=" << pid
                            << " status=" << status;
                    }
                }
                else    //子进程正常退出
                {
                    SYLAR_LOG_INFO(g_logger) << "child finished pid=" << pid;
                    break;
                }
                ProcessInfoMgr::GetInstance()->restart_count += 1;  //主(子)进程重启的次数
                sleep(g_daemon_restart_interval->getValue());   //等待时间重启
            }
        }
        return 0;
    }

    int start_daemon(int argc, char** argv  //启动程序可以选择是否使用守护进程的方式
                    , std::function<int(int argc, char** argv)> main_cb
                    , bool is_daemon)
    {
        if(!is_daemon)  //不采取守护进程
        {
            ProcessInfoMgr::GetInstance()->parent_id = getpid();    //父进程id  getpid当前进程
            ProcessInfoMgr::GetInstance()->parent_start_time = time(0); //父进程启动时间
            return real_start(argc, argv, main_cb);
        }
        return real_daemon(argc, argv, main_cb);
    }

}
