#include "application.h"

#include <unistd.h>
#include <signal.h>

#include "sylar_server/tcp_server.h"
#include "sylar_server/daemon.h"
#include "sylar_server/config.h"
#include "sylar_server/env.h"
#include "sylar_server/log.h"
// #include "sylar/module.h"
// #include "sylar/rock/rock_stream.h"
// #include "sylar/worker.h"
// #include "sylar/http/ws_server.h"
// #include "sylar/rock/rock_server.h"
// #include "sylar/ns/name_server_module.h"
// #include "sylar/db/fox_thread.h"
// #include "sylar/db/redis.h"

namespace sylar
{

    static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

    static sylar::ConfigVar<std::string>::ptr g_server_work_path =  //工作目录的路径
        sylar::Config::Lookup("server.work_path"
                ,std::string("/apps/work/sylar")
                , "server work path");

    static sylar::ConfigVar<std::string>::ptr g_server_pid_file =   //serverId文件
        sylar::Config::Lookup("server.pid_file"
                ,std::string("sylar.pid")
                , "server pid file");

    static sylar::ConfigVar<std::string>::ptr g_service_discovery_zk =
        sylar::Config::Lookup("service_discovery.zk"
                ,std::string("")
                , "service discovery zookeeper");

    //TCP服务器配置信息 (利用配置文件初始化TCP服务器配置信息)
    static sylar::ConfigVar<std::vector<TcpServerConf> >::ptr g_servers_conf
        = sylar::Config::Lookup("servers", std::vector<TcpServerConf>(), "http server config");

    Application* Application::s_instance = nullptr; //当前C++服务器启动类

    Application::Application()  //初始化
    {
        s_instance = this;
    }

    bool Application::init(int argc, char** argv)   //main函数参数解析
    {
        m_argc = argc;
        m_argv = argv;

        sylar::EnvMgr::GetInstance()->addHelp("s", "start with the terminal");
        sylar::EnvMgr::GetInstance()->addHelp("d", "run as daemon");
        sylar::EnvMgr::GetInstance()->addHelp("c", "conf path default: ./conf");
        sylar::EnvMgr::GetInstance()->addHelp("p", "print help");

        bool is_print_help = false;
        if(!sylar::EnvMgr::GetInstance()->init(argc, argv)) //main函数输入参数解析，获取进程路径(并插入参数)
        {
            is_print_help = true;   //解析失败，打印参数
        }

        if(sylar::EnvMgr::GetInstance()->has("p"))  //存在p————打印参数
        {
            is_print_help = true;
        }

        std::string conf_path = sylar::EnvMgr::GetInstance()->getConfigPath();  //获取配置文件的绝对路径
        SYLAR_LOG_INFO(g_logger) << "load conf path:" << conf_path;
        sylar::Config::LoadFromConfDir(conf_path);  //加载path文件夹里面的配置文件(如果配置文件没有变化，则不用加载该文件)

        // ModuleMgr::GetInstance()->init();
        // std::vector<Module::ptr> modules;
        // ModuleMgr::GetInstance()->listAll(modules);

        // for(auto i : modules)
        // {
        //     i->onBeforeArgsParse(argc, argv);
        // }

        if(is_print_help)
        {
            sylar::EnvMgr::GetInstance()->printHelp();
            return false;
        }

        // for(auto i : modules)
        // {
        //     i->onAfterArgsParse(argc, argv);
        // }
        // modules.clear();

        int run_type = 0;
        if(sylar::EnvMgr::GetInstance()->has("s"))  //非守护进程
        {
            run_type = 1;
        }
        if(sylar::EnvMgr::GetInstance()->has("d"))  //守护进程
        {
            run_type = 2;
        }

        if(run_type == 0)   //错误
        {
            sylar::EnvMgr::GetInstance()->printHelp();
            return false;
        }

        std::string pidfile = g_server_work_path->getValue()
                                    + "/" + g_server_pid_file->getValue();
        if(sylar::FSUtil::IsRunningPidfile(pidfile))    //判断进程pid文件是否存在且正在运行
        {
            SYLAR_LOG_ERROR(g_logger) << "server is running:" << pidfile;
            return false;
        }

        if(!sylar::FSUtil::Mkdir(g_server_work_path->getValue()))   //创建工作目录的路径
        {
            // SYLAR_LOG_INFO(g_logger)<<"a";
            SYLAR_LOG_FATAL(g_logger) << "create work path [" << g_server_work_path->getValue()
                << " errno=" << errno << " errstr=" << strerror(errno);
            return false;
        }
        return true;
    }

    bool Application::run() //守护进程方式
    {
        bool is_daemon = sylar::EnvMgr::GetInstance()->has("d");    //守护进程方式
        return start_daemon(m_argc, m_argv,
                std::bind(&Application::main, this, std::placeholders::_1,  //placeholders占位符(未指定数字对象)    _1用于代替回调函数中的第一个参数
                    std::placeholders::_2), is_daemon);
    }

    int Application::main(int argc, char** argv)    //服务器真正开始启动的函数
    {
        signal(SIGPIPE, SIG_IGN);
        SYLAR_LOG_INFO(g_logger) << "main";
        std::string conf_path = sylar::EnvMgr::GetInstance()->getConfigPath();  //获取配置文件的绝对路径
        sylar::Config::LoadFromConfDir(conf_path, true);    //加载path文件夹里面的配置文件(如果配置文件没有变化，则不用加载该文件)
        {
            std::string pidfile = g_server_work_path->getValue()
                                        + "/" + g_server_pid_file->getValue();
            std::ofstream ofs(pidfile); 
            if(!ofs)
            {
                SYLAR_LOG_ERROR(g_logger) << "open pidfile " << pidfile << " failed";
                return false;
            }
            ofs << getpid();    //写入当前服务器的进程ID(守护进程——子进程，没——父进程)到进程文件中
        }

        m_mainIOManager.reset(new sylar::IOManager(2, true, "main"));   //创建协程调度器
        m_mainIOManager->schedule(std::bind(&Application::run_fiber, this));    //run_fiber插入协程队列1
        m_mainIOManager->addTimer(2000, [](){   //插入epoll事件定时器
                // SYLAR_LOG_INFO(g_logger) << "hello";
        }, true);
        m_mainIOManager->stop();    //协程调度器关闭
        return 0;
    }

    int Application::run_fiber()    //处理TCP服务器配置信息，创建http等类型的TCP服务器，并启动服务器 (协程管理器的调用、数据管理)
    {
        // std::vector<Module::ptr> modules;
        // ModuleMgr::GetInstance()->listAll(modules);
        // bool has_error = false;
        // for(auto& i : modules)
        // {
        //     if(!i->onLoad())
        //     {
        //         SYLAR_LOG_ERROR(g_logger) << "module name="
        //             << i->getName() << " version=" << i->getVersion()
        //             << " filename=" << i->getFilename();
        //         has_error = true;
        //     }
        // }
        // if(has_error)
        // {
        //     _exit(0);
        // }

        // sylar::WorkerMgr::GetInstance()->init();
        // FoxThreadMgr::GetInstance()->init();
        // FoxThreadMgr::GetInstance()->start();
        // RedisMgr::GetInstance();

        auto http_confs = g_servers_conf->getValue();   //TCP服务器配置信息(http)     返回TcpServerConf结构体
        std::vector<TcpServer::ptr> svrs;   //成功创建的Tcp服务器集合
        for(auto& i : http_confs)   //遍历TcpServerConf结构体
        {
            SYLAR_LOG_DEBUG(g_logger) <<std::endl << LexicalCast<TcpServerConf, std::string>()(i); //把 TcpServerConf结构体 转化成 服务器配置文件

            std::vector<Address::ptr> address;  //存储服务器地址的address
            for(auto& a : i.address)    //处理服务器绑定地址，生成Address
            {
                size_t pos = a.find(":");

                //Unix地址 
                if(pos == std::string::npos)    //没有IP地址和端口号，则是Unix地址   
                {
                    SYLAR_LOG_ERROR(g_logger) << "invalid address: " << a;
                    address.push_back(UnixAddress::ptr(new UnixAddress(a)));    //是Unix地址
                    continue;
                }

                //IP地址
                int32_t port = atoi(a.substr(pos + 1).c_str()); //端口号    atoi字符串转整型
                //127.0.0.1
                auto addr = sylar::IPAddress::Create(a.substr(0, pos).c_str(), port);   //通过域名,IP地址,服务器名创建IPAddress
                if(addr)
                {
                    address.push_back(addr);
                    continue;
                }

                //网卡地址
                std::vector<std::pair<Address::ptr, uint32_t> > result; //保存指定网卡所有地址
                if(sylar::Address::GetInterfaceAddresses(result, a.substr(0, pos))) //获取指定网卡的地址和子网掩码位数
                {
                    for(auto& x : result)
                    {
                        auto ipaddr = std::dynamic_pointer_cast<IPAddress>(x.first);
                        if(ipaddr)
                        {
                            ipaddr->setPort(atoi(a.substr(pos + 1).c_str()));   //设置端口
                        }
                        address.push_back(ipaddr);
                    }
                    continue;
                }

                //host地址
                auto aaddr = sylar::Address::LookupAny(a);  //通过host地址返回对应条件的任意Address
                if(aaddr)
                {
                    address.push_back(aaddr);
                    continue;
                }
                SYLAR_LOG_ERROR(g_logger) << "invalid address: " << a;
                _exit(0);
            }
            
            IOManager* accept_worker = sylar::IOManager::GetThis(); //服务器Socket接收连接的调度器(接收连接)
            IOManager* io_worker = sylar::IOManager::GetThis(); //新连接的Socket(客户端)工作的调度器(处理传输数据)
            IOManager* process_worker = sylar::IOManager::GetThis();    //工作过程中需要新的IO协程调度器
            // if(!i.accept_worker.empty())
            // {
            //     accept_worker = sylar::WorkerMgr::GetInstance()->getAsIOManager(i.accept_worker).get();
            //     if(!accept_worker)
            //     {
            //         SYLAR_LOG_ERROR(g_logger) << "accept_worker: " << i.accept_worker
            //             << " not exists";
            //         _exit(0);
            //     }
            // }
            // if(!i.io_worker.empty())
            // {
            //     io_worker = sylar::WorkerMgr::GetInstance()->getAsIOManager(i.io_worker).get();
            //     if(!io_worker)
            //     {
            //         SYLAR_LOG_ERROR(g_logger) << "io_worker: " << i.io_worker
            //             << " not exists";
            //         _exit(0);
            //     }
            // }
            // if(!i.process_worker.empty())
            // {
            //     process_worker = sylar::WorkerMgr::GetInstance()->getAsIOManager(i.process_worker).get();
            //     if(!process_worker)
            //     {
            //         SYLAR_LOG_ERROR(g_logger) << "process_worker: " << i.process_worker
            //             << " not exists";
            //         _exit(0);
            //     }
            // }

            //处理创建什么类型(http)的TCP服务器
            TcpServer::ptr server;
            if(i.type == "http")    //http
            {
                server.reset(new sylar::http::HttpServer(i.keepalive,process_worker, io_worker, accept_worker));
            }
            // else if(i.type == "ws") //ws
            // {
            //     server.reset(new sylar::http::WSServer(process_worker, io_worker, accept_worker));
            // }
            // else if(i.type == "rock")   //rock
            // {
            //     server.reset(new sylar::RockServer("rock",process_worker, io_worker, accept_worker));
            // }
            // else if(i.type == "nameserver") //nameserver
            // {
            //     server.reset(new sylar::RockServer("nameserver",process_worker, io_worker, accept_worker));
            //     ModuleMgr::GetInstance()->add(std::make_shared<sylar::ns::NameServerModule>());
            // }
            else    //出错
            {
                SYLAR_LOG_ERROR(g_logger) << "invalid server type=" << i.type
                    << LexicalCast<TcpServerConf, std::string>()(i);
                _exit(0);
            }
            
            if(!i.name.empty()) //设置服务器名称
            {
                server->setName(i.name);
            }
            
            std::vector<Address::ptr> fails;    //存储绑定失败的地址
            if(!server->bind(address, fails, i.ssl))    //服务器绑定地址数组
            {
                for(auto& x : fails)
                {
                    SYLAR_LOG_ERROR(g_logger) << "bind address fail:"
                        << *x;
                }
                _exit(0);                
            }

            //测试添加servklet(可以不写)
            // sylar::http::HttpServer::ptr http=std::dynamic_pointer_cast<sylar::http::HttpServer>(server);
            // auto ad=http->getServletDispatch();
            // ad->addServlet("/sylar/xx", [](sylar::http::HttpRequest::ptr req    //添加精准匹配servlet(回调函数)
            //         ,sylar::http::HttpResponse::ptr rsp
            //         ,sylar::http::HttpSession::ptr session)
            //     {
            //     rsp->setBody(req->toString());
            //     return 0;
            //     });
            
            // if(i.ssl)   //是否使用https所有的证书
            // {
            //     if(!server->loadCertificates(i.cert_file, i.key_file))
            //     {
            //         SYLAR_LOG_ERROR(g_logger) << "loadCertificates fail, cert_file="
            //             << i.cert_file << " key_file=" << i.key_file;
            //     }
            // }

            server->setConf(i); //设置TCP服务器配置结构体
            m_servers[i.type].push_back(server);    //插入服务器
            svrs.push_back(server);
            
        }

        // if(!g_service_discovery_zk->getValue().empty())
        // {
        //     m_serviceDiscovery.reset(new ZKServiceDiscovery(g_service_discovery_zk->getValue()));
        //     m_rockSDLoadBalance.reset(new RockSDLoadBalance(m_serviceDiscovery));

        //     std::vector<TcpServer::ptr> svrs;
        //     if(!getServer("http", svrs))
        //     {
        //         m_serviceDiscovery->setSelfInfo(sylar::GetIPv4() + ":0:" + sylar::GetHostName());
        //     }
        //     else
        //     {
        //         std::string ip_and_port;
        //         for(auto& i : svrs)
        //         {
        //             auto socks = i->getSocks();
        //             for(auto& s : socks)
        //             {
        //                 auto addr = std::dynamic_pointer_cast<IPv4Address>(s->getLocalAddress());
        //                 if(!addr)
        //                 {
        //                     continue;
        //                 }
        //                 auto str = addr->toString();
        //                 if(str.find("127.0.0.1") == 0)
        //                 {
        //                     continue;
        //                 }
        //                 if(str.find("0.0.0.0") == 0)
        //                 {
        //                     ip_and_port = sylar::GetIPv4() + ":" + std::to_string(addr->getPort());
        //                     break;
        //                 }
        //                 else
        //                 {
        //                     ip_and_port = addr->toString();
        //                 }
        //             }
        //             if(!ip_and_port.empty())
        //             {
        //                 break;
        //             }
        //         }
        //         m_serviceDiscovery->setSelfInfo(ip_and_port + ":" + sylar::GetHostName());
        //     }
        // }

        // for(auto& i : modules)
        // {
        //     i->onServerReady();
        // }

        for(auto& i : svrs) //全部开启
        {
            i->start();
        }

        // if(m_rockSDLoadBalance)
        // {
        //     m_rockSDLoadBalance->start();
        // }

        // for(auto& i : modules)
        // {
        //     i->onServerUp();
        // }
        //ZKServiceDiscovery::ptr m_serviceDiscovery;
        //RockSDLoadBalance::ptr m_rockSDLoadBalance;
        //sylar::ZKServiceDiscovery::ptr zksd(new sylar::ZKServiceDiscovery("127.0.0.1:21811"));
        //zksd->registerServer("blog", "chat", sylar::GetIPv4() + ":8090", "xxx");
        //zksd->queryServer("blog", "chat");
        //zksd->setSelfInfo(sylar::GetIPv4() + ":8090");
        //zksd->setSelfData("vvv");
        //static RockSDLoadBalance::ptr rsdlb(new RockSDLoadBalance(zksd));
        //rsdlb->start();
        return 0;
    }

    bool Application::getServer(const std::string& type, std::vector<TcpServer::ptr>& svrs) //获取type类型的成功绑定地址服务器集合
    {
        auto it = m_servers.find(type);
        if(it == m_servers.end())
        {
            return false;
        }
        svrs = it->second;
        return true;
    }

    void Application::listAllServer(std::map<std::string, std::vector<TcpServer::ptr> >& servers)   //获取所有成功绑定地址服务器集合
    {
        servers = m_servers;
    }

}
