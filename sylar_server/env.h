//  对main函数输入参数解析，并存储到main函数输入参数类中————(并获取执行文件绝对路径、相对路径)
//(main函数 argc=参数名称个数+参数值个数+1  argv数组下标从1开始)
//  argv[0]是当前的执行进程(main函数)路径   ../bin/test_env.exe

//argv数组格式：
//默认参数格式  -config (-参数名称1) 
//              /path/to/config  (路径(参数值))
//             -file (-参数名称2) 
//              xxxx (参数值)
//              ...
#ifndef __SYLAR_ENV_H__
#define __SYLAR_ENV_H__

#include "sylar_server/singLeton.h"
#include "sylar_server/thread.h"
#include <map>
#include <vector>

namespace sylar
{

    /**
     * main函数输入参数类
    */
    class Env
    {
    public:

        typedef RWMutex RWMutexType;    //读写锁

        bool init(int argc, char** argv);   //main函数输入参数解析，获取进程路径(并插入参数)

        void add(const std::string& key, const std::string& val);   //增加参数
        bool has(const std::string& key);   //判断参数是否存在
        void del(const std::string& key);   //删除参数
        std::string get(const std::string& key, const std::string& default_value = ""); //获取参数对应的值  default_value默认值

        void addHelp(const std::string& key, const std::string& desc);  //增加用途信息
        void removeHelp(const std::string& key);    //删除用途信息
        void printHelp();   //打印用途信息

        const std::string& getExe() const { return m_exe;}  //获取绝对路径(真正路径)    /home/yangxin/sylar-master/bin/test_env.exe
        const std::string& getCwd() const { return m_cwd;}  //获取可执行文件的路径  /home/yangxin/sylar-master/bin/

        bool setEnv(const std::string& key, const std::string& val);    //设置环境变量
        std::string getEnv(const std::string& key, const std::string& default_value = "");  //获取环境变量

        std::string getAbsolutePath(const std::string& path) const; //获取path的绝对路径    path在可执行文件的路径中
        std::string getAbsoluteWorkPath(const std::string& path) const; //获取现在server工作的路径
        std::string getConfigPath();    //获取配置文件的绝对路径

    private:
        RWMutexType m_mutex;    //互斥量
        std::map<std::string, std::string> m_args;  // 1参数名称  2参数对应的值
        std::vector<std::pair<std::string, std::string> > m_helps;  // 1参数名称  2用途(应用信息描述)————参数名称可重复存在
                                                                    //  应用：是否使用某种功能

        std::string m_program;  //当前的执行进程(main函数)路径  ../bin/test_env.exe
        std::string m_exe;  //绝对路径(真正路径)    /home/yangxin/sylar-master/bin/test_env.exe
        std::string m_cwd;  //可执行文件的路径  /home/yangxin/sylar-master/bin/
    };

    typedef sylar::Singleton<Env> EnvMgr;   //单例模式封装

}

#endif
