//      Socket封装
//默认是IPv4
//TCP：
//  (不用使用socket函数，直接获取Address类对象(服务器、客户端地址)，设置端口)
//  (用Address类对象来创建Socket类对象，用connect，bind,listen,arrept函数就行，在发送、接收数据)
//
//  系统accept函数接受所有和服务器连接的客户端



//  m_sock代表服务器socket、客户端socket(但是不能同时代表服务器和客户端，也不能进行转换)
/**
 *  服务器socket(绑定IP地址):
 *      arrept()接收客户端连接——返回客户端socket(每一个socket对应一个客户端)——>用来处理数据
 * 
 *  客户端socket(没有IP地址)：
 *      connect()连接服务器——就可以使用客户端socket来处理数据
*/


#ifndef __SYLAR_SOCKET_H__
#define __SYLAR_SOCKET_H__

#include<memory>
#include<netinet/tcp.h>    //TCP_NODELAY不使用Nagle算法
#include<sys/types.h>
#include<sys/socket.h>
#include<openssl/err.h> //SSL_CTX
#include<openssl/ssl.h> //SSL

#include"address.h"
#include"noncopyable.h"

namespace sylar
{

    /**
     *  Socket封装类
     */
    class Socket : public std::enable_shared_from_this<Socket>,Noncopyable   //enable_shared_from_this<>成员函数可以获取自身类的智能指针,Time只能以智能指针形式存在
    {
    public:
        typedef std::shared_ptr<Socket> ptr;
        typedef std::weak_ptr<Socket> weak_ptr; //借助weak_ptr类型指针可以获取shared_ptr指针的一些状态信息(比如引用计数)
    
        /**
         *  Socket类型
         */
        enum Type
        {
            TCP = SOCK_STREAM,  //TCP类型
            UDP = SOCK_DGRAM    //UDP类型
        };

        /**
         *  Socket协议簇
         */
        enum Family
        {
            IPv4 = AF_INET,     //IPv4 socket
            IPv6 = AF_INET6,    //IPv6 socket
            UNIX = AF_UNIX,     //Unix socket
        };

        /**
         *  创建TCP Socket(满足地址类型)
         *      address 地址
         */
        static Socket::ptr CreateTCP(sylar::Address::ptr address);

        /**
         *  创建UDP Socket(满足地址类型)
         *      address 地址
         */
        static Socket::ptr CreateUDP(sylar::Address::ptr address);

        static Socket::ptr CreateTCPSocket();   //创建IPv4的TCP Socket

        static Socket::ptr CreateUDPSocket();   //创建IPv4的UDP Socket

        static Socket::ptr CreateTCPSocket6();  //创建IPv6的TCP Socket

        static Socket::ptr CreateUDPSocket6();  //创建IPv6的UDP Socket

        static Socket::ptr CreateUnixTCPSocket();   //创建Unix的TCP Socket

        static Socket::ptr CreateUnixUDPSocket();   //创建Unix的UDP Socket

        /**
         *  Socket构造函数
         *      family 协议簇(IP协议)
         *      type socket类型
         *      protocol 协议
         */
        Socket(int family, int type, int protocol = 0);

        /**
         *  析构函数
         */
        virtual ~Socket();

        int64_t getSendTimeout();   //获取发送超时时间(毫秒)

        void setSendTimeout(int64_t v); //设置发送超时时间(毫秒)    v选项的值

        int64_t getRecvTimeout();   //获取接受超时时间(毫秒)

        void setRecvTimeout(int64_t v); //设置接受超时时间(毫秒)    v选项的值

        /**
         *  获取sockopt  getsockopt获取一个套接字的选项
         *      level 协议层次
         *      option 选项名。
         *      result 指向某个变量的指针，该变量是要获取选项的值。
         *      len result的长度。
         */
        bool getOption(int level, int option, void* result, socklen_t* len);
        
        /**
         *  获取sockopt模板  getsockopt获取一个套接字的选项
         *      level 协议层次
         *      option 选项名。
         *      value 指向某个变量的指针，该变量是要获取选项的值。
         */
        template<class T>
        bool getOption(int level, int option, T& result)
        {
            socklen_t length = sizeof(T);
            return getOption(level, option, &result, &length);
        }

        /**
         *  设置sockopt  setsockopt设置套接字描述符的属性
         *      level 协议层次
         *      option 选项名。
         *      result 指向某个变量的指针，该变量是要设置选项的值。
         *      len result的长度。
         */
        bool setOption(int level, int option, const void* result, socklen_t len);
        
        /**
         *  设置sockopt模板  setsockopt设置套接字描述符的属性
         *      level 协议层次
         *      option 选项名。
         *      value 指向某个变量的指针，该变量是要设置选项的值。
         */
        template<class T>
        bool setOption(int level, int option, const T& value)
        {
            return setOption(level, option, &value, sizeof(T));
        }
        
        /**
         *  服务器接收connect(客户端)链接
         *      成功返回新连接的socket(客户端),失败返回nullptr
         *  (前提:Socket必须 bind , listen(监听) 成功)
         */
        virtual Socket::ptr accept();

        /**
         *  服务器绑定IP地址
         *      addr 地址
         *  (是否绑定成功)
         */
        virtual bool bind(const Address::ptr addr);

        /**
         *  客户端连接服务器地址
         *      addr 目标地址(服务器)
         *      timeout_ms 超时时间(毫秒)
         */
        virtual bool connect(const Address::ptr addr, uint64_t timeout_ms = -1);

        /**
         * @brief 重新客户端连接服务器地址
         * @param[in] timeout_ms 超时时间(毫秒)
        */
        virtual bool reconnect(uint64_t timeout_ms = -1);

        /**
         *  监听socket客户端连接
         *      backlog 未完成连接队列的最大长度(监听个数)
         *  (返回监听是否成功)
         *  (必须先 bind 成功)
         */
        virtual bool listen(int backlog = SOMAXCONN);

        /**
         *  关闭socket
         */
        virtual bool close();

        /**
         *  发送数据(TCP)
         *      buffer 待发送数据的存储
         *      length 待发送数据的长度
         *      flags 标志字(0)
         * 返回:
         *          >0 发送成功对应大小的数据
         *          =0 socket被关闭
         *          <0 socket出错
         */
        virtual int send(const void* buffer, size_t length, int flags = 0);
        
        /**
         *  发送数据(TCP)
         *      buffers 待发送数据的存储(iovec数组——向量元素)
         *      length 待发送数据的长度(iovec长度)
         *      flags 标志字
         * 返回:
         *          >0 发送成功对应大小的数据
         *          =0 socket被关闭
         *          <0 socket出错
         */
        virtual int send(const iovec* buffers, size_t length, int flags = 0);
        
        /**
         *  发送数据(UDP)
         *      buffer 待发送数据的存储
         *      length 待发送数据的长度
         *      to 发送的目标地址
         *      flags 标志字
         * 返回:
         *          >0 发送成功对应大小的数据
         *          =0 socket被关闭
         *          <0 socket出错
         */
        virtual int sendTo(const void* buffer, size_t length, const Address::ptr to, int flags = 0);
        
        /**
         *  发送数据(UDP)
         *      buffers 待发送数据的存储(iovec数组——向量元素)
         *      length 待发送数据的长度(iovec长度)
         *      to 发送的目标地址
         *      flags 标志字
         * 返回:
         *          >0 发送成功对应大小的数据
         *          =0 socket被关闭
         *          <0 socket出错
         */
        virtual int sendTo(const iovec* buffers, size_t length, const Address::ptr to, int flags = 0);
        
        /**
         *  接受数据(TCP)
         *      buffer 接收数据的存储
         *      length 接收数据的内存大小
         *      flags 标志字
         * 返回:
         *          >0 接收到对应大小的数据
         *          =0 socket被关闭
         *          <0 socket出错
         */
        virtual int recv(void* buffer, size_t length, int flags = 0);
        
        /**
         *  接受数据(TCP)
         *      buffers 接收数据的存储(iovec数组——向量元素)
         *      length 接收数据的内存大小(iovec数组长度)
         *      flags 标志字
         * 返回:
         *          >0 接收到对应大小的数据
         *          =0 socket被关闭
         *          <0 socket出错
         */
        virtual int recv(iovec* buffers, size_t length, int flags = 0);

        /**
         *  接受数据(UDP)
         *      buffer 接收数据的存储
         *      length 接收数据的内存大小
         *      from 发送端地址
         *      flags 标志字
         * 返回:
         *          >0 接收到对应大小的数据
         *          =0 socket被关闭
         *          <0 socket出错
         */
        virtual int recvFrom(void* buffer, size_t length, Address::ptr from, int flags = 0);

        /**
         *  接受数据(UDP)
         *      buffers 接收数据的存储(iovec数组——向量元素)
         *      length 接收数据的内存大小(iovec数组长度)
         *      from 发送端地址
         *      flags 标志字
         * 返回:
         *          >0 接收到对应大小的数据
         *          =0 socket被关闭
         *          <0 socket出错
         */
        virtual int recvFrom(iovec* buffers, size_t length, Address::ptr from, int flags = 0);

        Address::ptr getRemoteAddress();    //获取远端地址(本机：服务器——客户器地址     本机：客户器——服务器地址)

        Address::ptr getLocalAddress(); //获取本地地址(本机：服务器——服务器地址     本机：客户器——客户器地址)

        int getSocket() const { return m_sock;} //返回socket句柄

        int getFamily() const { return m_family; }  //获取协议簇(IP协议)

        int getType() const { return m_type; }  //获取socket类型

        int getProtocol() const { return m_protocol; }  //获取协议

        bool isConnected() const { return m_isConnected; }  //返回是否连接成功

        bool isValid() const;   //m_sock是否有效(m_sock != -1)

        int getError(); //返回Socket的错误

        virtual std::ostream& dump(std::ostream& os) const; //输出socket基本信息到流中

        virtual std::string toString() const;   //socket基本信息字符串化

        bool cancelRead();  //取消读(唤醒执行)

        bool cancelWrite(); //取消写(唤醒执行)

        bool cancelAccept();    //取消accept(唤醒执行)

        bool cancelAll();   //取消所有事件(唤醒执行)

    protected:

        void initSock();    ////初始化socket(m_sock)

        void newSock(); //创建新的socket(赋予到m_sock)

        /**
         *  初始化sock(sock赋予到m_sock)
         *      sock 建立Socket的socket句柄
         *  (返回初始化是否成功)
         */
        virtual bool init(int sock);

    protected:

        int m_sock; //socket句柄
        int m_family;   //协议簇(IP协议)
        int m_type; //socket类型    流格式套接字TCP（SOCK_STREAM） 数据报格式套接字UDP（SOCK_DGRAM）
        int m_protocol; //协议
        bool m_isConnected; //是否连接成功
        Address::ptr m_localAddress;    //本地地址(本机：服务器——服务器地址     本机：客户器——客户器地址)
        Address::ptr m_remoteAddress;   //远端地址(本机：服务器——客户器地址     本机：客户器——服务器地址)
    };

    // class SSLSocket : public Socket
    // {
    // public:
    //     typedef std::shared_ptr<SSLSocket> ptr;

    //     static SSLSocket::ptr CreateTCP(sylar::Address::ptr address);
    //     static SSLSocket::ptr CreateTCPSocket();
    //     static SSLSocket::ptr CreateTCPSocket6();

    //     SSLSocket(int family, int type, int protocol = 0);
    //     virtual Socket::ptr accept() override;
    //     virtual bool bind(const Address::ptr addr) override;
    //     virtual bool connect(const Address::ptr addr, uint64_t timeout_ms = -1) override;
    //     virtual bool listen(int backlog = SOMAXCONN) override;
    //     virtual bool close() override;
    //     virtual int send(const void* buffer, size_t length, int flags = 0) override;
    //     virtual int send(const iovec* buffers, size_t length, int flags = 0) override;
    //     virtual int sendTo(const void* buffer, size_t length, const Address::ptr to, int flags = 0) override;
    //     virtual int sendTo(const iovec* buffers, size_t length, const Address::ptr to, int flags = 0) override;
    //     virtual int recv(void* buffer, size_t length, int flags = 0) override;
    //     virtual int recv(iovec* buffers, size_t length, int flags = 0) override;
    //     virtual int recvFrom(void* buffer, size_t length, Address::ptr from, int flags = 0) override;
    //     virtual int recvFrom(iovec* buffers, size_t length, Address::ptr from, int flags = 0) override;

    //     bool loadCertificates(const std::string& cert_file, const std::string& key_file);
    //     virtual std::ostream& dump(std::ostream& os) const override;
    // protected:
    //     virtual bool init(int sock) override;
    // private:
    //     std::shared_ptr<SSL_CTX> m_ctx;
    //     std::shared_ptr<SSL> m_ssl;
    // };

    /**
     *  流式输出socket
     *      os 输出流
     *      sock Socket类
     */
    std::ostream& operator<<(std::ostream& os, const Socket& sock);

}

#endif