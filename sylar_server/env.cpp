#include "env.h"
#include "sylar_server/log.h"
#include <string.h>
#include <iostream>
#include <iomanip>
#include <unistd.h>
#include <stdlib.h>
#include "config.h"

namespace sylar
{

    static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

    bool Env::init(int argc, char** argv)   //main函数输入参数解析，获取进程路径(并插入参数)
    {
        char link[1024] = {0};  //链接路径
        char path[1024] = {0};  //真正路径
        sprintf(link, "/proc/%d/exe", getpid());    //获取链接路径  sprintf()拼接到link中   getpid()获取当前进程ID
        readlink(link, path, sizeof(path)); //读取软链接路径指向的真正路径  readlink将link的符号链接内容存储到path所指的内存空间
        // /path/xxx/exe
        m_exe = path;   //绝对路径(真正路径)

        auto pos = m_exe.find_last_of("/"); //find_last_of逆向查找在原字符串中最后一个与指定字符串（或字符）中的某个字符匹配的字符，返回它的位置
        m_cwd = m_exe.substr(0, pos) + "/"; //可执行文件的路径  substr复制子字符串

        m_program = argv[0];    //当前的执行进程(main函数)路径
        // -config 参数1
        //  /path/to/config
        // -file 参数2
        //  xxxx 
        // -d 参数3
        const char* now_key = nullptr;  //上一个参数
        for(int i = 1; i < argc; ++i)
        {
            if(argv[i][0] == '-')   //是参数名称
            {
                if(strlen(argv[i]) > 1) //存在参数名称
                {
                    if(now_key) //上一个参数，没有参数值    now_key上一个参数名称
                    {
                        add(now_key, "");
                    }
                    now_key = argv[i] + 1;  //当前参数名称  argv[i]+1 ——> argv[i][1]
                }
                else
                {
                    SYLAR_LOG_ERROR(g_logger) << "invalid arg idx=" << i
                        << " val=" << argv[i];
                    return false;
                }
            }
            else    //是参数值
            {
                if(now_key)
                {
                    add(now_key, argv[i]);  //增加参数值
                    now_key = nullptr;
                }
                else
                {
                    SYLAR_LOG_ERROR(g_logger) << "invalid arg idx=" << i
                        << " val=" << argv[i];
                    return false;
                }
            }
        }
        if(now_key) //上一个参数，没有参数值    now_key上一个参数名称
        {
            add(now_key, "");
        }
        return true;
    }

    void Env::add(const std::string& key, const std::string& val)   //增加参数
    {
        RWMutexType::WriteLock lock(m_mutex);
        m_args[key] = val;
    }

    bool Env::has(const std::string& key)   //判断参数是否存在
    {
        RWMutexType::ReadLock lock(m_mutex);
        auto it = m_args.find(key);
        return it != m_args.end();
    }

    void Env::del(const std::string& key)   //删除参数
    {
        RWMutexType::WriteLock lock(m_mutex);
        m_args.erase(key);
    }

    std::string Env::get(const std::string& key, const std::string& default_value)  //获取参数对应的值
    {
        RWMutexType::ReadLock lock(m_mutex);
        auto it = m_args.find(key);
        return it != m_args.end() ? it->second : default_value;
    }

    void Env::addHelp(const std::string& key, const std::string& desc)  //增加用途信息
    {
        removeHelp(key);    //先删除，再增加
        RWMutexType::WriteLock lock(m_mutex);
        m_helps.push_back(std::make_pair(key, desc));
    }

    void Env::removeHelp(const std::string& key)    //删除用途信息
    {
        RWMutexType::WriteLock lock(m_mutex);
        for(auto it = m_helps.begin();it != m_helps.end();)
        {
            if(it->first == key)
            {
                it = m_helps.erase(it); //erase返回下一个迭代器
            }
            else
            {
                ++it;
            }
        }
    }

    void Env::printHelp()   //打印用途信息
    {
        RWMutexType::ReadLock lock(m_mutex);
        std::cout << "Usage: " << m_program << " [options]" << std::endl;
        for(auto& i : m_helps)
        {
            std::cout << std::setw(5) << "-" << i.first << " : " << i.second << std::endl;
        }
    }

    bool Env::setEnv(const std::string& key, const std::string& val)    //设置环境变量
    {
        return !setenv(key.c_str(), val.c_str(), 1);
    }

    std::string Env::getEnv(const std::string& key, const std::string& default_value)   //获取环境变量
    {
        const char* v = getenv(key.c_str());
        if(v == nullptr)
        {
            return default_value;
        }
        return v;
    }

    std::string Env::getAbsolutePath(const std::string& path) const //获取path的绝对路径    path在可执行文件的路径中
    {
        if(path.empty())    //根路径
        {
            return "/";
        }
        if(path[0] == '/')  //本来就是绝对路径
        {
            return path;
        }
        return m_cwd + path;    //绝对路径=可执行文件路径+相对路径
    }

    std::string Env::getAbsoluteWorkPath(const std::string& path) const //获取现在server工作的路径
    {
        if(path.empty())    //根路径
        {
            return "/";
        }
        if(path[0] == '/')  //本来就是绝对路径
        {
            return path;
        }
        static sylar::ConfigVar<std::string>::ptr g_server_work_path =
            sylar::Config::Lookup<std::string>("server.work_path");
        return g_server_work_path->getValue() + "/" + path;
    }

    std::string Env::getConfigPath()    //获取配置文件的绝对路径
    {
        return getAbsolutePath(get("c", "conf"));
    }

}
