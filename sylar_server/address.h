//  网络地址的封装(IPv4,IPv6,Unix)
//(识别IP地址是IPv4,IPv6,Unix那种类型)
//创建对应的Address(包含端口号，端口号区分用什么协议)
//Address根据相对应的端口号在应用层协议池中寻找对应的协议
#ifndef __SYLAR_ADDRESS_H__
#define __SYLAR_ADDRESS_H__

#include <memory>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h> //sockaddr_un
#include <arpa/inet.h>  //sockaddr_in...
#include <unistd.h>
#include <iostream>
#include <vector>
#include <map>

namespace sylar
{
    class IPAddress;    //只是声明，不是编译

    /**
     *  网络地址的基类,抽象类
     */
    class Address
    {
    public:
        typedef std::shared_ptr<Address> ptr;

        /**
         *  通过sockaddr指针创建Address
         *      addr sockaddr指针
         *      addrlen sockaddr的长度
         *  (返回和sockaddr相匹配的Address,失败返回nullptr)
         */
        static Address::ptr Create(const sockaddr* addr, socklen_t addrlen);

        /**
         *  通过host地址返回对应条件的所有Address,并插入容器中(DNS域名服务器作用，可以解析网上的网站--要联网 http:80)
         *      result 保存满足条件的Address(容器)
         *      host 域名,服务器名等.举例: www.sylar.top[:80] (方括号为可选内容)
         *          (IPv4   xx.xx.xx.xx:端口号  IPv6    [xx:xx:xx:xx:xx:xx:xx:xx]:端口号)
         *      family 协议族(AF_INT, AF_INT6, AF_UNIX)
         *      type socketl类型SOCK_STREAM、SOCK_DGRAM 等
         *      protocol 协议,IPPROTO_TCP、IPPROTO_UDP 等
         *  (返回是否转换成功)
         */
        static bool Lookup(std::vector<Address::ptr>& result, const std::string& host,
                int family = AF_INET, int type = 0, int protocol = 0);

        /**
         *  通过host地址返回对应条件的任意Address(DNS域名服务器作用，可以解析网上的网站--要联网 http:80)
         *      host 域名,服务器名等.举例: www.sylar.top[:80] (方括号为可选内容)
         *          (IPv4   xx.xx.xx.xx:端口号  IPv6    [xx:xx:xx:xx:xx:xx:xx:xx]:端口号)
         *      family 协议族(AF_INT, AF_INT6, AF_UNIX)
         *      type socketl类型SOCK_STREAM、SOCK_DGRAM 等
         *      protocol 协议,IPPROTO_TCP、IPPROTO_UDP 等
         *  (返回满足条件的任意Address,失败返回nullptr)
         */
        static Address::ptr LookupAny(const std::string& host,
                int family = AF_INET, int type = 0, int protocol = 0);

        /**
         *  通过host地址返回对应条件的任意IPAddress(DNS域名服务器作用，可以解析网上的网站--要联网 http:80)
         *      host 域名,服务器名等.举例: www.sylar.top[:80] (方括号为可选内容)
         *          (IPv4   xx.xx.xx.xx:端口号  IPv6    [xx:xx:xx:xx:xx:xx:xx:xx]:端口号)
         *      family 协议族(AF_INT, AF_INT6, AF_UNIX)
         *      type socketl类型SOCK_STREAM、SOCK_DGRAM 等
         *      protocol 协议,IPPROTO_TCP、IPPROTO_UDP 等
         *  (返回满足条件的任意IPAddress,失败返回nullptr)
         */
        static std::shared_ptr<IPAddress> LookupAnyIPAddress(const std::string& host,
                int family = AF_INET, int type = 0, int protocol = 0);

        /**
         *  返回本机所有网卡的<网卡名, 地址, 子网掩码位数>到result
         *      result 保存本机所有地址
         *          (网络接口名,<Address结构体,子网掩码1的数量>)
         *          (multimap、map是二叉树，map的key不可重复，multimap可以pair对组，成对出现的数据)
         *      family 协议族(AF_INT, AF_INT6, AF_UNIX)
         *  (是否获取成功)
         */
        static bool GetInterfaceAddresses(std::multimap<std::string
                        ,std::pair<Address::ptr, uint32_t> >& result,
                        int family = AF_INET);

        /**
         *  获取指定网卡的地址和子网掩码位数
         *      result 保存指定网卡所有地址
         *              (Address结构体,子网掩码1的数量)
         *              (pair对组，成对出现的数据)
         *      iface 网卡名称
         *      family 协议族(AF_INT, AF_INT6, AF_UNIX)
         *  (是否获取成功)
         */
        static bool GetInterfaceAddresses(std::vector<std::pair<Address::ptr, uint32_t> >&result
                        ,const std::string& iface, int family = AF_INET);

        /**
         *  虚析构函数
         */
        virtual ~Address(){}

        /**
         *  返回协议簇
         */
        int getFamily() const;

        /**
         *  返回sockaddr指针,只读
         */
        virtual const sockaddr* getAddr() const=0;
        
        /**
         *  返回sockaddr指针,读写
         */
        virtual sockaddr* getAddr() = 0;

        /**
         *  返回sockaddr的长度
         */
        virtual socklen_t getAddrLen() const=0;

        /**
         *  流式输出地址、端口号
         */
        virtual std::ostream& insert(std::ostream& os) const=0;

        /**
         *  返回可读性字符串
         */
        std::string toString();

        bool operator<(const Address& rhs) const;   //小于号比较仿函数
        bool operator==(const Address& rhs) const;  //等于仿函数
        bool operator!=(const Address& rhs) const;  //不等于仿函数

    };

    /**
     *  IP地址的基类
     */
    class IPAddress : public Address
    {
    public:
        typedef std::shared_ptr<IPAddress> ptr;

        /**
         *  通过域名,IP地址,服务器名创建IPAddress
         *      address 域名,IP地址,服务器名等.举例: www.sylar.top、(192.1.1.1)、(x:x:x:x:x:x:x:x)
         *      port 端口号
         *  (调用成功返回IPAddress,失败返回nullptr)
         */
        static IPAddress::ptr Create(const char* address, uint16_t port = 0);

        /**
         *  获取该地址的广播地址
         *      prefix_len 子网掩码位数(1的位数)
         *  (调用成功返回IPAddress,失败返回nullptr)
         */
        virtual IPAddress::ptr broadcastAddress(uint32_t prefix_len) = 0;

        /**
         *  获取该地址的网段(IP地址不变=任意一个IP地址)
         *      prefix_len 子网掩码位数
         *  (调用成功返回IPAddress,失败返回nullptr)
         */
        virtual IPAddress::ptr networdAddress(uint32_t prefix_len) = 0;
    
        /**
         *  获取子网掩码地址
         *      prefix_len 子网掩码位数
         *  (调用成功返回IPAddress,失败返回nullptr)
         */
        virtual IPAddress::ptr subnetMask(uint32_t prefix_len) = 0;

        virtual uint32_t getPort() const = 0;   //返回端口号

        virtual void setPort(uint16_t v) = 0;   //设置端口号

    };

    /**
     *  IPv4地址
     */
    class IPv4Address : public IPAddress
    {
    public:
        typedef std::shared_ptr<IPv4Address> ptr;

        /**
         *  使用点分十进制地址创建IPv4Address
         *      address 点分十进制地址,如:192.168.1.1
         *      port 端口号
         *  返回IPv4Address,失败返回nullptr
         */
        static IPv4Address::ptr Create(const char* address, uint16_t port = 0);

        /**
         *  通过sockaddr_in构造IPv4Address
         *      address sockaddr_in结构体
         */
        IPv4Address(const sockaddr_in& address);

        /**
         *  构造函数，通过二进制地址构造IPv4Address
         *      address 二进制地址address
         *      port 端口号
         */
        IPv4Address(uint32_t address = INADDR_ANY, uint16_t port = 0);

        //Address纯虚函数
        const sockaddr* getAddr() const override;   //返回sockaddr指针,只读
        sockaddr* getAddr() override;   //返回sockaddr指针,读写
        socklen_t getAddrLen() const override;  //返回sockaddr的长度
        std::ostream& insert(std::ostream& os) const override;  //流式输出地址、端口号

        //IPAddress纯虚函数
        IPAddress::ptr broadcastAddress(uint32_t prefix_len) override;  //获取该地址的广播地址
        IPAddress::ptr networdAddress(uint32_t prefix_len) override;    //获取该地址的网段(IP地址不变=任意一个IP地址)
        IPAddress::ptr subnetMask(uint32_t prefix_len) override;    //获取子网掩码地址
        uint32_t getPort() const override;  //返回端口号
        void setPort(uint16_t v) override;  //设置端口号

    private:
        sockaddr_in m_addr; //sockaddr地址(sockaddr_in IPv4的结构体)
    };

    /**
     *  IPv6地址
     */
    class IPv6Address : public IPAddress
    {
    public:
        typedef std::shared_ptr<IPv6Address> ptr;

        /**
         *  通过IPv6地址字符串构造IPv6Address
         *      address IPv6地址字符串,如xx:xx:xx:xx:xx:xx:xx:xx
         *      port 端口号
         */
        static IPv6Address::ptr Create(const char* address, uint16_t port = 0);

        /**
         *  无参构造函数
         */
        IPv6Address();

        /**
         *  通过sockaddr_in6构造IPv6Address
         *      address sockaddr_in6结构体
         */
        IPv6Address(const sockaddr_in6& address);

        /**
         *  通过IPv6地址字符串构造IPv6Address
         *      address IPv6地址字符串
         *      port 端口号
         */
        IPv6Address(const uint8_t address[16], uint16_t port = 0);

        //Address纯虚函数
        const sockaddr* getAddr() const override;   //返回sockaddr指针,只读
        sockaddr* getAddr() override;   //返回sockaddr指针,读写
        socklen_t getAddrLen() const override;  //返回sockaddr的长度
        std::ostream& insert(std::ostream& os) const override;  //流式输出地址、端口号

        //IPAddress纯虚函数
        IPAddress::ptr broadcastAddress(uint32_t prefix_len) override;  //获取该地址的广播地址
        IPAddress::ptr networdAddress(uint32_t prefix_len) override;    //获取该地址的网段(IP地址不变=任意一个IP地址)
        IPAddress::ptr subnetMask(uint32_t prefix_len) override;    //获取子网掩码地址
        uint32_t getPort() const override;  //返回端口号
        void setPort(uint16_t v) override;  //设置端口号
    private:
        sockaddr_in6 m_addr; //sockaddr地址(sockaddr_in6 IPv6的结构体)
    };

    /**
     *  Unix Socket地址
     *  (Unix socket（也称为 Unix 域套接字）是一种用于同一台主机上进程间通信（IPC）的机制,
     *  与主机间的进程通信不同，它不是通过 “IP地址 + TCP或UDP端口号” 的方式进程通信，而是使用 socket 类型的文件来完成通信)
     */
    class UnixAddress : public Address
    {
    public:
        typedef std::shared_ptr<UnixAddress> ptr;

        /**
         *  无参构造函数
         */
        UnixAddress();

        /**
         *  通过路径构造UnixAddress
         *      path UnixSocket路径(长度小于UNIX_PATH_MAX)
         *  (网络编程的socket地址是IP地址加端口号，而UNIX Domain Socket的地址是一个socket类型的文件在文件系统中的路径，)
         */
        UnixAddress(const std::string& path);
        
        //Address纯虚函数
        const sockaddr* getAddr() const override;   //返回sockaddr指针,只读
        sockaddr* getAddr() override;   //返回sockaddr指针,读写
        socklen_t getAddrLen() const override;  //返回sockaddr的长度
        std::ostream& insert(std::ostream& os) const override;  //流式输出socket类型的文件在文件系统中的路径

        void setAddrLen(uint32_t v);    //设置sockaddr的长度
        std::string getPath() const;    //返回路径
    private:
        sockaddr_un m_addr; //sockaddr地址(sockaddr_un Unix的结构体)
        socklen_t m_length; //sockaddr地址结构体长度
    };

    /**
     *  未知地址
     */
    class UnknownAddress : public Address
    {
    public:
        typedef std::shared_ptr<UnknownAddress> ptr;

        /**
         *  通过sa_family构造UnknownAddress
         *      family 协议族
        */
        UnknownAddress(int family);

        /**
         *  通过sockaddr构造UnknownAddress
         *      addr sockaddr结构体
         */
        UnknownAddress(const sockaddr& addr);
        
        //Address纯虚函数
        const sockaddr* getAddr() const override;   //返回sockaddr指针,只读
        sockaddr* getAddr() override;   //返回sockaddr指针,读写
        socklen_t getAddrLen() const override;  //返回sockaddr的长度
        std::ostream& insert(std::ostream& os) const override;  //流式输出地址、端口号
    private:
        sockaddr m_addr;    //sockaddr地址(sockaddr 未知类型的结构体)
    };

    /**
     *  流式输出Address
     */
    std::ostream& operator<<(std::ostream& os, const Address& addr);

}

#endif