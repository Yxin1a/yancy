#include "sylar_server/daemon.h"
#include "sylar_server/iomanager.h"
#include "sylar_server/log.h"

static sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

sylar::Timer::ptr timer;    //定时器

int server_main(int argc, char** argv)  //server函数
{
    SYLAR_LOG_INFO(g_logger) << sylar::ProcessInfoMgr::GetInstance()->toString();

    sylar::IOManager iom(1);

    timer = iom.addTimer(1000, [](){
            SYLAR_LOG_INFO(g_logger) << "onTimer";
            static int count = 0;
            if(++count > 10)
            {
                exit(1);
            }
    }, true);
    return 0;
}

int main(int argc, char** argv)
{
    std::cout<<argc<<std::endl;
    return sylar::start_daemon(argc, argv, server_main, argc != 1);
}
