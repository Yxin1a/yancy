//  C++服务器框架的调用(服务器框架结合调用实现)

//  添加main函数参数
//  -s  非守护进程
//  -d  守护进程
//  -p  打印参数
#ifndef __SYLAR_APPLICATION_H__
#define __SYLAR_APPLICATION_H__

#include "sylar_server/http/http_server.h"
// #include "sylar-master/streams/service_discovery.h"
// #include "sylar_server/rock/rock_stream.h"

namespace sylar
{

    /**
     *  当前C++服务器启动类
    */
    class Application
    {
    public:
        Application();  //初始化

        static Application* GetInstance() { return s_instance;} //获取当前C++服务器启动类
        bool init(int argc, char** argv);   //main函数参数解析
        bool run(); //守护进程方式

        bool getServer(const std::string& type, std::vector<TcpServer::ptr>& svrs); //返回type类型的成功绑定地址服务器集合
        void listAllServer(std::map<std::string, std::vector<TcpServer::ptr> >& servers);   //获取所有成功绑定地址服务器集合

        // ZKServiceDiscovery::ptr getServiceDiscovery() const { return m_serviceDiscovery;}   //获取zookeeper实现服务注册与发现
        // RockSDLoadBalance::ptr getRockSDLoadBalance() const { return m_rockSDLoadBalance;}  //获取Rock协议负载均衡管理基类
    private:
        int main(int argc, char** argv);    //服务器真正开始启动的函数
        int run_fiber();    //处理TCP服务器配置信息，创建http等类型的TCP服务器，并启动服务器 (协程管理器的调用、数据管理)
    private:
        int m_argc = 0; //参数个数
        char** m_argv = nullptr;    //参数数组

        std::vector<sylar::http::HttpServer::ptr> m_httpservers;
        std::map<std::string, std::vector<TcpServer::ptr> > m_servers;  //成功绑定地址服务器集合    1服务器类型HTTP     2该类型服务器集合
        IOManager::ptr m_mainIOManager; //当前基于Epoll的IO协程调度器
        static Application* s_instance; //当前C++服务器启动类

        // /// zookeeper实现服务注册与发现
        // ZKServiceDiscovery::ptr m_serviceDiscovery;
        // /// Rock协议负载均衡管理基类
        // RockSDLoadBalance::ptr m_rockSDLoadBalance;
    };

}

#endif
