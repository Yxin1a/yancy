//  TCP服务器的封装
//TCP服务器的
//  ①绑定地址
//  ②监听客户端的连接
//  ③接受客户端的连接 插入到调度协程中(安排空闲的线程处理 接受客户端的连接)
//  ④对该客户端的数据传输 插入到调度协程中(安排空闲的线程处理 该客户端的数据传输)
//(③和④的协程调度器     不一样：则是协程调度器嵌套      一样：则是使用同一个协程调度器，来先后执行)
#ifndef __SYLAR_TCP_SERVER_H__
#define __SYLAR_TCP_SERVER_H__

#include<memory>
#include<functional>
#include"address.h"
#include"iomanager.h"
#include"socket.h"
#include"noncopyable.h"
#include"config.h"

namespace sylar
{

    /**
     *  TCP服务器配置结构体
    */
    struct TcpServerConf
    {
        typedef std::shared_ptr<TcpServerConf> ptr;

        std::vector<std::string> address;   //服务器绑定地址
        int keepalive = 0;  //是否长连接
        int timeout = 1000 * 2 * 60;    //连接超时时间
        int ssl = 0;    //是否使用https所有的证书
        std::string id; //服务器id
        /// 服务器类型，http, ws, rock
        std::string type = "http";  //服务器类型
        std::string name;   //服务器名称
        std::string cert_file;
        std::string key_file;
        std::string accept_worker;  //服务器使用的IO协程调度器(连接使用的协程调度器)
        std::string io_worker;  //新连接的Socket(客户端)工作的调度器(处理传输数据)
        std::string process_worker; //工作过程中需要新的IO协程调度器
        std::map<std::string, std::string> args;

        bool isValid() const    //返回地址数组是否为空
        {
            return !address.empty();
        }

        bool operator==(const TcpServerConf& oth) const //判断自定义类型是否相等
        {
            return address == oth.address
                && keepalive == oth.keepalive
                && timeout == oth.timeout
                && name == oth.name
                && ssl == oth.ssl
                && cert_file == oth.cert_file
                && key_file == oth.key_file
                && accept_worker == oth.accept_worker
                && io_worker == oth.io_worker
                && process_worker == oth.process_worker
                && args == oth.args
                && id == oth.id
                && type == oth.type;
        }
    };

    /**
     *  类型转换模板类片特化(YAML String 转换成 TcpServerConf)
     *      (把服务器配置文件 转化成 TcpServerConf结构体)
     */
    template<>
    class LexicalCast<std::string, TcpServerConf>
    {
    public:
        TcpServerConf operator()(const std::string& v)
        {
            YAML::Node node = YAML::Load(v);
            TcpServerConf conf;
            conf.id = node["id"].as<std::string>(conf.id);  //conf.id默认值
            conf.type = node["type"].as<std::string>(conf.type);
            conf.keepalive = node["keepalive"].as<int>(conf.keepalive);
            conf.timeout = node["timeout"].as<int>(conf.timeout);
            conf.name = node["name"].as<std::string>(conf.name);
            conf.ssl = node["ssl"].as<int>(conf.ssl);
            conf.cert_file = node["cert_file"].as<std::string>(conf.cert_file);
            conf.key_file = node["key_file"].as<std::string>(conf.key_file);
            conf.accept_worker = node["accept_worker"].as<std::string>();
            conf.io_worker = node["io_worker"].as<std::string>();
            conf.process_worker = node["process_worker"].as<std::string>();
            conf.args = LexicalCast<std::string
                ,std::map<std::string, std::string> >()(node["args"].as<std::string>(""));
            if(node["address"].IsDefined()) //插入服务器的绑定地址
            {
                for(size_t i = 0; i < node["address"].size(); ++i)
                {
                    conf.address.push_back(node["address"][i].as<std::string>());
                }
            }
            return conf;
        }
    };

    /**
     *  类型转换模板类片特化(TcpServerConf 转换成 YAML String)
     *      (把 TcpServerConf结构体 转化成 服务器配置文件)
     */
    template<>
    class LexicalCast<TcpServerConf, std::string>
    {
    public:
        std::string operator()(const TcpServerConf& conf)
        {
            YAML::Node node;
            node["id"] = conf.id;
            node["type"] = conf.type;
            node["name"] = conf.name;
            node["keepalive"] = conf.keepalive;
            node["timeout"] = conf.timeout;
            node["ssl"] = conf.ssl;
            node["cert_file"] = conf.cert_file;
            node["key_file"] = conf.key_file;
            node["accept_worker"] = conf.accept_worker;
            node["io_worker"] = conf.io_worker;
            node["process_worker"] = conf.process_worker;
            node["args"] = YAML::Load(LexicalCast<std::map<std::string, std::string>
                , std::string>()(conf.args));
            for(auto& i : conf.address)
            {
                node["address"].push_back(i);
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    /**
     *  TCP服务器封装
     */
    class TcpServer : public std::enable_shared_from_this<TcpServer>, Noncopyable//enable_shared_from_this<>成员函数可以获取自身类的智能指针,Time只能以智能指针形式存在
    {
    public:
        typedef std::shared_ptr<TcpServer> ptr;
        
        /**
         *  构造函数
         *      worker socket客户端工作的协程调度器(连接使用的协程调度器)
         *      io_worker 新连接的Socket(客户端)工作的调度器(处理传输数据)
         *      accept_worker 服务器socket执行接收socket连接的协程调度器(服务器使用的IO协程调度器)
         */
        TcpServer(sylar::IOManager* worker = sylar::IOManager::GetThis()
                ,sylar::IOManager* io_worker = sylar::IOManager::GetThis()
                ,sylar::IOManager* accept_worker = sylar::IOManager::GetThis());

        /**
         *  析构函数
         */
        virtual ~TcpServer();

        /**
         *  绑定地址(并开启监听)
         *      ssl 是否使用https所有的证书
         * return:
         *      返回是否绑定成功
         */
        virtual bool bind(sylar::Address::ptr addr, bool ssl = false);

        /**
         *  绑定地址数组(并开启监听)
         *      addrs 需要绑定的地址数组
         *      fails 绑定失败的地址
         *      ssl 是否使用https所有的证书
         * return:
         *      是否绑定成功
         */
        virtual bool bind(const std::vector<Address::ptr>& addrs
                            ,std::vector<Address::ptr>& fails
                            ,bool ssl = false);

        /**
         *  启动服务，并安排线程处理接受连接客户端连接
         * 前提：
         *      需要bind成功后执行
         */
        virtual bool start();

        /**
         *  停止服务
         */
        virtual void stop();

        uint64_t getRecvTimeout() const { return m_recvTimeout;}    //返回服务器读取超时时间(毫秒)
        void setRecvTimeout(uint64_t v) { m_recvTimeout = v;}   //设置服务器读取超时时间(毫秒)
        std::string getName() const { return m_name;}   //返回服务器名称
        bool isStop() const { return m_isStop;} //返回服务器是否停止
        TcpServerConf::ptr getConf() const { return m_conf;}    //返回TCP服务器配置结构体
        void setConf(TcpServerConf::ptr v) { m_conf = v;}   //设置TCP服务器配置结构体
        std::vector<Socket::ptr> getSocks() const { return m_socks;}    //返回监听Socket数组(正在监听客户端的socket)

        void setConf(const TcpServerConf& v);   //设置TCP服务器配置结构体

        //bool loadCertificates(const std::string& cert_file, const std::string& key_file);

        virtual void setName(const std::string& v) { m_name = v;}   //设置服务器名称

        virtual std::string toString(const std::string& prefix = "");   //以字符串形式返回

    protected:

        virtual void handleClient(Socket::ptr client);  //处理新连接的Socket类(处理新连接的客户端数据传输)

        /**
         *  开始接受连接客户端连接，并安排线程处理
         *      sock 服务器socket
        */
        virtual void startAccept(Socket::ptr sock);

    protected:
        std::vector<Socket::ptr> m_socks;   //监听Socket数组(正在监听客户端的socket)
        IOManager* m_worker;    //正在Socket工作的调度器
        IOManager* m_ioWorker;  //新连接的Socket(客户端)工作的调度器(处理传输数据)
        IOManager* m_acceptWorker;  //服务器Socket接收连接的调度器(接收连接)

        uint64_t m_recvTimeout; //服务器接收超时时间(毫秒)————接收数据之类的
        std::string m_name; //服务器名称
        std::string m_type = "tcp"; //服务器类型(tcp、udp)
        bool m_isStop;  //服务器是否停止

        bool m_ssl = false; //是否使用https所有的证书

        TcpServerConf::ptr m_conf;  //TCP服务器配置结构体
    };

}

#endif