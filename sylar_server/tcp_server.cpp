#include"tcp_server.h"
#include"config.h"
#include"log.h"

namespace sylar
{
    //设置配置接收超时时间(毫秒)2分钟
    static sylar::ConfigVar<uint64_t>::ptr g_tcp_server_read_timeout =
        sylar::Config::Lookup("tcp_server.read_timeout", (uint64_t)(60 * 1000 * 2), //
                "tcp server read timeout");

    static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

    // TcpServer

    TcpServer::TcpServer(sylar::IOManager* worker,sylar::IOManager* io_worker,sylar::IOManager* accept_worker)  //初始化
        :m_worker(worker)
        ,m_ioWorker(io_worker)
        ,m_acceptWorker(accept_worker)
        ,m_recvTimeout(g_tcp_server_read_timeout->getValue())
        ,m_name("sylar/1.0.0")
        ,m_isStop(true)
    {}

    TcpServer::~TcpServer() //释放内存
    {
        for(auto& i : m_socks)
        {
            i->close();
        }
        m_socks.clear();
    }

    bool TcpServer::bind(sylar::Address::ptr addr, bool ssl)    //绑定地址(无论什么类型的地址都行)，并开启监听
    {
        std::vector<Address::ptr> addrs;    //需要绑定的地址数组
        std::vector<Address::ptr> fails;    //绑定失败的地址数组
        addrs.push_back(addr);
        return bind(addrs, fails, ssl);
    }

    bool TcpServer::bind(const std::vector<Address::ptr>& addrs,std::vector<Address::ptr>& fails,bool ssl)  //绑定地址数组，并开启监听
    {
        m_ssl = ssl;
        for(auto& addr : addrs) //遍历需要绑定的地址数组
        {
            //Socket::ptr sock = ssl ? SSLSocket::CreateTCP(addr) : Socket::CreateTCP(addr);
            Socket::ptr sock = Socket::CreateTCP(addr);
            if(!sock->bind(addr))   //服务器绑定IP地址
            {
                SYLAR_LOG_ERROR(g_logger) << "bind fail errno="
                    << errno << " errstr=" << strerror(errno)
                    << " addr=[" << addr->toString() << "]";
                fails.push_back(addr);
                continue;
            }
            if(!sock->listen()) //监听socket客户端连接
            {
                SYLAR_LOG_ERROR(g_logger) << "listen fail errno="
                    << errno << " errstr=" << strerror(errno)
                    << " addr=[" << addr->toString() << "]";
                fails.push_back(addr);
                continue;
            }
            m_socks.push_back(sock);    //监听Socket数组(正在监听客户端的socket)插入
        }

        if(!fails.empty())  //不为空
        {
            m_socks.clear();
            return false;
        }

        for(auto& i : m_socks)  //打印成功(正在监听客户端的socket)
        {
            SYLAR_LOG_INFO(g_logger) << "type=" << m_type
                << " name=" << m_name
                << " ssl=" << m_ssl
                << " server bind success: " << *i;
        }
        return true;
    }

    bool TcpServer::start() //启动服务，并安排线程处理接受连接客户端连接
    {
        if(!m_isStop)   //服务已经开始了
        {
            return true;
        }
        m_isStop = false;
        for(auto& sock : m_socks)   //正在监听客户端的socket
        {
            m_acceptWorker->schedule(std::bind(&TcpServer::startAccept, //处理新连接的Socket类 插入调度协程，安排空闲的线程处理该客户端的数据
                        shared_from_this(), sock)); //在子协程中处理新连接的Socket类    shared_from_this拿出自己的智能指针
        }
        return true;
    }
    
    /**
     *  停止服务
     */
    void TcpServer::stop()  //停止服务
    {
        m_isStop = true;    //服务器停止
        auto self = shared_from_this(); //shared_from_this拿出自己的智能指针
        m_acceptWorker->schedule([this, self]()
        {
            for(auto& sock : m_socks)
            {
                sock->cancelAll();  //取消所有事件(唤醒执行)
                sock->close();  //释放
            }
            m_socks.clear();    //释放
        });
    }
    
    void TcpServer::setConf(const TcpServerConf& v) //设置TCP服务器配置结构体
    {
        m_conf.reset(new TcpServerConf(v));
    }
    
    // bool TcpServer::loadCertificates(const std::string& cert_file, const std::string& key_file)
    // {
    //     for(auto& i : m_socks)
    //     {
    //         auto ssl_socket = std::dynamic_pointer_cast<SSLSocket>(i);
    //         if(ssl_socket)
    //         {
    //             if(!ssl_socket->loadCertificates(cert_file, key_file))
    //             {
    //                 return false;
    //             }
    //         }
    //     }
    //     return true;
    // }
    
    std::string TcpServer::toString(const std::string& prefix)  //以字符串形式返回
    {
        std::stringstream ss;
        ss  << prefix << "[type=" << m_type
            << " name=" << m_name << " ssl=" << m_ssl
            << " worker=" << (m_worker ? m_worker->getName() : "")
            << " accept=" << (m_acceptWorker ? m_acceptWorker->getName() : "")
            << " recv_timeout=" << m_recvTimeout << "]" << std::endl;
        std::string pfx = prefix.empty() ? "    " : prefix;
        for(auto& i : m_socks)
        {
            ss << pfx << pfx << *i << std::endl;
        }
        return ss.str();
    }
    
    void TcpServer::handleClient(Socket::ptr client)  //处理新连接的Socket类(处理新连接的客户端数据传输)
    {
        SYLAR_LOG_INFO(g_logger) << "handleClient: "<< *client;
    }
    
    void TcpServer::startAccept(Socket::ptr sock) //开始接受连接客户端连接，并安排线程处理
    {
        while(!m_isStop)    //服务器已开启，就不停地接受客户端
        {
            Socket::ptr client = sock->accept();    //客户端socket
            if(client)  //连接成功
            {
                client->setRecvTimeout(m_recvTimeout);
                m_ioWorker->schedule(std::bind(&TcpServer::handleClient,    //处理新连接的Socket类 插入调度协程，安排空闲的线程处理该客户端的数据
                            shared_from_this(), client));   //在子协程中处理新连接的Socket类    shared_from_this拿出自己的智能指针
            }
            else    //失败
            {
                SYLAR_LOG_ERROR(g_logger) << "accept errno=" << errno
                    << " errstr=" << strerror(errno);
            }
        }
    }
    
}