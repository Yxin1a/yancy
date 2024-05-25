#include "sylar_server/env.h"
#include <unistd.h>
#include <iostream>
#include <fstream>

//利用/proc/进程号/cmdline和全局变量构造函数，实现在进入main函数前解析参数
struct A
{
    A()
    {
        std::ifstream ifs("/proc/" + std::to_string(getpid()) + "/cmdline", std::ios::binary);  //打开/proc/当前进程/cmdline文件
        std::string content;
        content.resize(4096);

        ifs.read(&content[0], content.size());  //读取数据
        content.resize(ifs.gcount());

        for(size_t i = 0; i < content.size(); ++i)  //0为路径和参数的分隔符
        {
            std::cout << i << " - " << content[i] << " - " << (int)content[i] << std::endl;
        }
    }
};

A a;

int main(int argc, char** argv)
{
    std::cout << "argc=" << argc << std::endl;
    sylar::EnvMgr::GetInstance()->addHelp("s", "start with the terminal");  //非守护进程
    sylar::EnvMgr::GetInstance()->addHelp("d", "run as daemon");    //守护进程
    sylar::EnvMgr::GetInstance()->addHelp("p", "print help");
    if(!sylar::EnvMgr::GetInstance()->init(argc, argv)) //main函数输入参数解析失败
    {
        sylar::EnvMgr::GetInstance()->printHelp();
        return 0;
    }

    std::cout << "exe=" << sylar::EnvMgr::GetInstance()->getExe() << std::endl;
    std::cout << "cwd=" << sylar::EnvMgr::GetInstance()->getCwd() << std::endl;

    std::cout << "path=" << sylar::EnvMgr::GetInstance()->getEnv("PATH", "xxx") << std::endl;
    std::cout << "test=" << sylar::EnvMgr::GetInstance()->getEnv("TEST", "") << std::endl;
    std::cout << "set env " << sylar::EnvMgr::GetInstance()->setEnv("TEST", "yy") << std::endl;
    std::cout << "test=" << sylar::EnvMgr::GetInstance()->getEnv("TEST", "") << std::endl;
    if(sylar::EnvMgr::GetInstance()->has("p"))
    {
        sylar::EnvMgr::GetInstance()->printHelp();
    }
    return 0;
}
