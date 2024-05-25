//0xff  0x是十六进制 ff为11111111  255
#include "address.h"
#include "log.h"
#include <sstream>
#include <netdb.h>  //addrinfo
#include <ifaddrs.h>    //ifaddrs
#include <stddef.h>

#include "endian.h"

namespace sylar
{

    static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");
    
    template<class T>
    static T CreateMask(uint32_t bits)  //子网掩码反码
    {
        return (1 << (sizeof(T) * 8 - bits)) - 1;   //bits=8 结果：111111111111111111111111 ->24个1 前面没数，不参与运算
    }

    template<class T>
    static uint32_t CountBytes(T value) //统计子网掩码1的数量，(效率是最高的)
    {
        uint32_t result = 0;
        for(; value; ++result)
        {
            value &= value - 1; //-1，有一个地方1变成0，&之后就变成0，  其他地方也有0变成1，但是&之后还是原来的数
        }
        return result;
    }

    //Address
    Address::ptr Address::Create(const sockaddr* addr, socklen_t addrlen) //通过sockaddr指针创建Address对应的IPv4、IPv6
    {
        if(addr == nullptr) //不存在
        {
            return nullptr;
        }

        Address::ptr result;
        switch(addr->sa_family)
        {
            case AF_INET:   //IPv4
                result.reset(new IPv4Address(*(const sockaddr_in*)addr));
                break;
            case AF_INET6:  //IPv6
                result.reset(new IPv6Address(*(const sockaddr_in6*)addr));
                break;
            default:    //未知
                result.reset(new UnknownAddress(*addr));
                break;
        }
        return result;
    }

    bool Address::Lookup(std::vector<Address::ptr>& result, const std::string& host,int family, int type, int protocol) //通过host地址(IP地址，域名,服务器名)返回对应条件的所有Address，并插入容器中
    {
        addrinfo hints, *results, *next;    //addrinfo是链表
        hints.ai_flags = 0; //AI_PASSIVE	1	被动的，用于bind，通常用于server socket
                            // AI_CANONNAME	2	用于返回主机的规范名称
                            // AI_NUMERICHOST	4	地址为数字串
        hints.ai_family = family;   //地址族    AF_INET--ipv4   AF_INET6--ipv6  AF_UNSPEC--协议无关
        hints.ai_socktype = type;   //SOCK_STREAM	1	数据流协议  (socket类型)
                                    //SOCK_DGRAM	2	数据报协议
        hints.ai_protocol = protocol;   //IPPROTO_IP	0	IP协议  (协议)
                                        // IPPROTO_IPV4	4	IPv4
                                        // IPPROTO_IPV6	41	IPv6
                                        // IPPROTO_UDP	17	UDP
                                        // IPPROTO_TCP	6	TCP
        hints.ai_addrlen = 0;   //addr的长度
        hints.ai_canonname = NULL;  //服务名(端口号、服务器名)
        hints.ai_addr = NULL;   //address
        hints.ai_next = NULL;   //下一个节点

        std::string node;   //IP地址
        const char* service = NULL; //IP地址    个别字符

        //检查 ipv6address service(端口号)
        if(!host.empty() && host[0] == '[') //是ipv6返回对应的Address   [xx:xx:xx:xx:xx:xx:xx:xx]:端口号    
        {
            const char* endipv6 = (const char*)memchr(host.c_str() + 1, ']', host.size() - 1);   //host.c_str() + 1所指向的字符串的前host.size() - 1个字节中搜索第一次出现字符]（一个无符号字符）的位置
            if(endipv6)
            {
                if(*(endipv6 + 1) == ':')
                {
                    service = endipv6 + 2;  //端口号
                }
                node = host.substr(1, endipv6 - host.c_str() - 1);  //提取子字符串 xx:xx:xx:xx:xx:xx:xx:xx
            }
        }

        //检查 node service(端口号)
        if(node.empty())    //不是IPv6  就是xx.xx.xx.xx:端口号  www.sylar.top[:80]
        {
            service = (const char*)memchr(host.c_str(), ':', host.size());  //host.c_str() + 1所指向的字符串的前host.size()个字节中搜索第一次出现字符：（一个无符号字符）的位置
            if(service) //:存在
            {
                if(!memchr(service + 1, ':', host.c_str() + host.size() - service - 1)) //service + 1,所指向的字符串的前host.c_str() + host.size() - service - 1个字节中搜索第一次出现字符：（一个无符号字符）的位置
                {   //:后面不能有在有:
                    node = host.substr(0, service - host.c_str());  //提取子字符串 xx.xx.xx.xx 或 www.sylar.top
                    ++service;  //端口号
                }
            }
        }

        if(node.empty())    //都不是IPv6、IPv4、域名    就是服务器名
        {
            node = host;    //整个地址
        }
        int error = getaddrinfo(node.c_str(), service, &hints, &results);   //getaddrinfo函数能够处理名字到地址以及服务到端口这两种转换(DNS域名服务器作用，可以解析网上的网站--要联网 http:80)
                                                                            //(返回一个指向struct addrinfo结构链表的指针(node.c_str(), service和hints结合赋予results)，要释放内存，会造成内存泄漏。)
                                                                            //  address 域名,IP地址,服务器名
                                                                            //  服务名(端口号、服务器名称)，如果此参数设置为NULL，那么返回的socket地址中的端口号不会被设置
                                                                            //  用户设定的 struct addrinfo 结构体(用户定义的结构体)
                                                                            //  存储结果的 struct addrinfo 结构体指针
        if(error)
        {
            SYLAR_LOG_DEBUG(g_logger) << "Address::Lookup getaddress(" << host << ", "
                << family << ", " << type << ") err=" << error << " errstr="
                << gai_strerror(error);
            return false;
        }

        //解析
        next = results;
        while(next)
        {
            result.push_back(Create(next->ai_addr, (socklen_t)next->ai_addrlen));   //创建ai_addr对应的address,插入容器
            next = next->ai_next;
        }

        freeaddrinfo(results);  //释放内存
        return !result.empty();
    }

    Address::ptr Address::LookupAny(const std::string& host,int family, int type, int protocol) //通过host地址(IP地址，域名,服务器名)返回对应条件的任意Address
    {
        std::vector<Address::ptr> result;
        if(Lookup(result, host, family, type, protocol))
        {
            return result[0];   //返回的不是容器，是第一个Address智能指针
        }
        return nullptr;
    }

    IPAddress::ptr Address::LookupAnyIPAddress(const std::string& host,int family, int type, int protocol)  //通过host地址(IP地址，域名,服务器名)返回对应条件的任意IPAddress
    {
        std::vector<Address::ptr> result;
        if(Lookup(result, host, family, type, protocol))
        {
            for(auto& i : result)
            {
                IPAddress::ptr v = std::dynamic_pointer_cast<IPAddress>(i); //dynamic_pointer_cast将一个基类智能指针转化为继承类智能指针
                if(v)   //是IPv4 或 IPv6 的
                {
                    return v;   //返回的不是容器，是一个IPAddress智能指针
                }
            }
        }
        return nullptr;
    }

    bool Address::GetInterfaceAddresses(std::multimap<std::string,std::pair<Address::ptr, uint32_t> >& result,int family)   //返回本机所有网卡的<网卡名, 地址, 子网掩码位数>，并插入容器中
    {
        struct ifaddrs *next, *results;
        if(getifaddrs(&results) != 0)   //getifaddrs()获取本地网络接口的信息。(创建一个链表results，链表上的每个节点都是一个struct ifaddrs结构)
        {   //失败
            SYLAR_LOG_DEBUG(g_logger) << "Address::GetInterfaceAddresses getifaddrs "
                " err=" << errno << " errstr=" << strerror(errno);
            return false;
        }

        try
        {
            for(next = results; next; next = next->ifa_next)    //ifa_next链表中下一个struct ifaddr结构
            {
                Address::ptr addr;  //Address结构体
                uint32_t prefix_len = ~0u;  //子网掩码1的数量
                if(family != AF_UNSPEC && family != next->ifa_addr->sa_family)  //不是Unix && 不是对应的
                {   //ifa_addr包含网络地址的sockaddr结构
                    continue;
                }
                switch(next->ifa_addr->sa_family)   //网络地址的协议族
                {
                    case AF_INET:   //IPv4
                        {
                            addr = Create(next->ifa_addr, sizeof(sockaddr_in));
                            uint32_t netmask = ((sockaddr_in*)next->ifa_netmask)->sin_addr.s_addr;  //ifa_netmask网络掩码(子网掩码)的结构  取出子网掩码
                            prefix_len = CountBytes(netmask);   //子网掩码1的数量
                        }
                        break;
                    case AF_INET6:  //IPv6
                        {
                            addr = Create(next->ifa_addr, sizeof(sockaddr_in6));
                            in6_addr& netmask = ((sockaddr_in6*)next->ifa_netmask)->sin6_addr;  //ifa_netmask网络掩码(子网掩码)的结构  取出子网掩码
                            prefix_len = 0;
                            for(int i = 0; i < 16; ++i) //16*8=128，分开来算
                            {
                                prefix_len += CountBytes(netmask.s6_addr[i]);   //子网掩码1的数量
                            }
                        }
                        break;
                    default:
                        break;
                }

                if(addr)
                {
                    result.insert(std::make_pair(next->ifa_name,    //ifa_name网络接口名
                                std::make_pair(addr, prefix_len))); //addr--Address结构体  prefix_len子网掩码1的数量
                }
            }
        }
        catch (...)
        {
            SYLAR_LOG_ERROR(g_logger) << "Address::GetInterfaceAddresses exception";
            freeifaddrs(results);   //释放
            return false;
        }
        freeifaddrs(results);   //释放
        return !result.empty(); //获取成功
    }

    bool Address::GetInterfaceAddresses(std::vector<std::pair<Address::ptr, uint32_t> >&result,const std::string& iface, int family)    //获取指定网卡的地址和子网掩码位数
    {
        if(iface.empty() || iface == "*")   //任意网卡 或 没有指名网卡
        {
            if(family == AF_INET || family == AF_UNSPEC)    //返回0.0.0.0任意地址
            {
                result.push_back(std::make_pair(Address::ptr(new IPv4Address()), 0u));  //pair对组  ①可以直接调用make_pair生成pair对象 ②接受隐式的类型转换
            }
            if(family == AF_INET6 || family == AF_UNSPEC)   //返回0:0:0:0:0:0:0:0任意地址
            {
                result.push_back(std::make_pair(Address::ptr(new IPv6Address()), 0u));  //pair对组  ①可以直接调用make_pair生成pair对象 ②接受隐式的类型转换
            }
            return true;
        }

        std::multimap<std::string
            ,std::pair<Address::ptr, uint32_t> > results;

        if(!GetInterfaceAddresses(results, family)) //返回本机所有网卡的<网卡名, 地址, 子网掩码位数>
        {
            return false;   //不存在该网卡
        }

        auto its = results.equal_range(iface);  //查找key等于目标值的所有元素,返回first和second之间
        for(; its.first != its.second; ++its.first)
        {
            result.push_back(its.first->second);    //插入指定网卡信息
        }
        return !result.empty(); //获取成功
    }

    int Address::getFamily() const //返回协议簇
    {
        return getAddr()->sa_family;    //sa_family协议簇
    }

    std::string Address::toString()  //返回可读性字符串
    {
        std::stringstream ss;
        insert(ss); //流式输出地址、端口号
        return ss.str();
    }

    bool Address::operator<(const Address& rhs) const   //小于号比较仿函数
    {
        socklen_t minlen=std::min(getAddrLen(),rhs.getAddrLen());   //min取出最小值  max取出最大值
        int result=memcmp(getAddr(),rhs.getAddr(),minlen);  //比较内存区域this和rhs的前minlen个字节。
        if(result<0)    //getAddr() < rhs.getAddr()
        {
            return true;
        }
        else if(result>0)   //getAddr() > rhs.getAddr()
        {
            return false;
        }
        else if(getAddrLen()<rhs.getAddrLen())  //前面字节都相等，比较sockaddr的长度
        {
            return true;
        }
        return false;
    }
    
    bool Address::operator==(const Address& rhs) const  //等于仿函数
    {
        return getAddrLen()==rhs.getAddrLen()   //sockaddr的长度相等
            && memcmp(getAddr(),rhs.getAddr(),getAddrLen())==0; //内存区域this和rhs的字节都相等
    }

    bool Address::operator!=(const Address& rhs) const  //不等于仿函数
    {
        return !(*this==rhs);
    }

    //IPAddress
    IPAddress::ptr IPAddress::Create(const char* address, uint16_t port)    //通过域名,IP地址,服务器名创建IPAddress
    {
        addrinfo hints, *results;
        memset(&hints, 0, sizeof(addrinfo));    //清零

        hints.ai_flags = AI_NUMERICHOST;    //AI_NUMERICHOST:标志禁止任何可能冗长的网络主机地址查找
        hints.ai_family = AF_UNSPEC;    //AF_UNSPEC协议无关 IPv4、IPv6都可以

        int error = getaddrinfo(address, NULL, &hints, &results);   //getaddrinfo函数能够处理名字到地址以及服务到端口这两种转换(DNS域名服务器作用，可以解析网上的网站--要联网 http:80)
                                                                    //(返回一个指向struct addrinfo结构链表的指针(node.c_str(), service和hints结合赋予results)，要释放内存，会造成内存泄漏。)
                                                                    //  address 域名,IP地址,服务器名
                                                                    //  服务名(端口号、服务器名称)，如果此参数设置为NULL，那么返回的socket地址中的端口号不会被设置
                                                                    //  用户设定的 struct addrinfo 结构体(用户定义的结构体)
                                                                    //  存储结果的 struct addrinfo 结构体指针
        if(error)   //失败
        {
            SYLAR_LOG_DEBUG(g_logger) << "IPAddress::Create(" << address
                << ", " << port << ") error=" << error
                << " errno=" << errno << " errstr=" << strerror(errno);
            return nullptr;
        }

        try
        {
            IPAddress::ptr result = std::dynamic_pointer_cast<IPAddress>(   //dynamic_pointer_cast将一个基类智能指针转化为继承类智能指针
                    Address::Create(results->ai_addr, (socklen_t)results->ai_addrlen));
            if(result)
            {
                result->setPort(port);
            }
            freeaddrinfo(results);  //freeaddrinfo()将getaddrinfo返回的所有动态获取的存储空间通过调用freeaddrinfo返回给系统==>释放
            return result;
        }
        catch (...)
        {
            freeaddrinfo(results);  //freeaddrinfo()释放
            return nullptr;
        }
    }

    //IPv4Address
    IPv4Address::ptr IPv4Address::Create(const char* address, uint16_t port)  //使用点分十进制地址(192.168.1.1)创建IPv4Address
    {
        IPv4Address::ptr rt(new IPv4Address);
        rt->m_addr.sin_port = byteswapOnLittleEndian(port); //端口号字节转换
        int result = inet_pton(AF_INET, address, &rt->m_addr.sin_addr); //IP地址
        if(result <= 0) //失败
        {
            SYLAR_LOG_DEBUG(g_logger) << "IPv4Address::Create(" << address << ", "
                    << port << ") rt=" << result << " errno=" << errno
                    << " errstr=" << strerror(errno);
            return nullptr;
        }
        return rt;
    }

    IPv4Address::IPv4Address(const sockaddr_in& address)    //通过sockaddr_in构造IPv4Address,初始化sockaddr_in
    {
        m_addr=address;
    }

    IPv4Address::IPv4Address(uint32_t address, uint16_t port)   //初始化sockaddr_in，通过二进制地址构造IPv4Address
    {
        memset(&m_addr,0,sizeof(m_addr));   //清零————将m_addr中当前位置后面的sizeof(m_addr)个字节用0替换并返回m_addr
        m_addr.sin_family=AF_INET;  //sin_family协议族  AF_INET IPv4
        m_addr.sin_port=byteswapOnLittleEndian(port);   //字节序转换    sin_port端口号
        m_addr.sin_addr.s_addr=byteswapOnLittleEndian(address); //字节序转换    二进制地址address
    }

    const sockaddr* IPv4Address::getAddr() const    //返回sockaddr指针,只读
    {
        return (sockaddr*)&m_addr;
    }
    
    sockaddr* IPv4Address::getAddr()    //返回sockaddr指针,读写
    {
        return (sockaddr*)&m_addr;
    }
    
    socklen_t IPv4Address::getAddrLen() const   //返回sockaddr的长度
    {
        return sizeof(m_addr);
    }

    std::ostream& IPv4Address::insert(std::ostream& os) const   //流式输出地址、端口号
    {
        uint32_t addr=byteswapOnLittleEndian(m_addr.sin_addr.s_addr);   //取出IPv4地址
        os << ((addr >> 24) & 0xff) << "."
           << ((addr >> 16) & 0xff) << "."
           << ((addr >> 8) & 0xff) << "."
           << (addr & 0xff);    //以十进制的方式将IPv4地址存进流输出(xx.xx.xx.xx:端口号)
        os << ":" << byteswapOnLittleEndian(m_addr.sin_port);   //将端口号存进流输出
        return os;
    }

    IPAddress::ptr IPv4Address::broadcastAddress(uint32_t prefix_len)   //获取该地址的广播地址
    {
        if(prefix_len>32)   //超过IPv4的位数
        {
            return nullptr;
        }
        sockaddr_in baddr(m_addr);  //拷贝
        baddr.sin_addr.s_addr |= byteswapOnLittleEndian(CreateMask<uint32_t>(prefix_len));  //网络地址中的网络地址部分不变，主机地址变为全1，结果就是广播地址。
        return IPv4Address::ptr(new IPv4Address(baddr));
    }   

    IPAddress::ptr IPv4Address::networdAddress(uint32_t prefix_len) //获取该地址的网段(IP地址不变=任意一个IP地址)
    {
        if(prefix_len>32)
        {
            return nullptr;
        }
        sockaddr_in baddr(m_addr);  //拷贝
        baddr.sin_addr.s_addr &= byteswapOnLittleEndian(CreateMask<uint32_t>(prefix_len));  //IP地址与子网掩码反码进行&运算，得到的结果就是网络地址
        return IPv4Address::ptr(new IPv4Address(baddr));
    }

    IPAddress::ptr IPv4Address::subnetMask(uint32_t prefix_len) //获取子网掩码地址
    {
        sockaddr_in subnet;
        memset(&subnet, 0, sizeof(subnet));
        subnet.sin_family = AF_INET;
        subnet.sin_addr.s_addr = ~byteswapOnLittleEndian(CreateMask<uint32_t>(prefix_len)); //子网掩码反码的反码==子网掩码
        return IPv4Address::ptr(new IPv4Address(subnet));
    }

    uint32_t IPv4Address::getPort() const   //返回端口号
    {
        return byteswapOnLittleEndian(m_addr.sin_port);
    }
    
    void IPv4Address::setPort(uint16_t v)   //设置端口号
    {
        m_addr.sin_port=byteswapOnLittleEndian(v);
    }

    //IPv6Address
    IPv6Address::ptr IPv6Address::Create(const char* address, uint16_t port)    //通过IPv6地址字符串(xx:xx:xx:xx:xx:xx:xx:xx)构造IPv6Address
    {
        IPv6Address::ptr rt(new IPv6Address);
        rt->m_addr.sin6_port = byteswapOnLittleEndian(port);    //端口号字节转换
        int result = inet_pton(AF_INET6, address, &rt->m_addr.sin6_addr);   //IP地址
        if(result <= 0) //失败
        {
            SYLAR_LOG_DEBUG(g_logger) << "IPv6Address::Create(" << address << ", "
                    << port << ") rt=" << result << " errno=" << errno
                    << " errstr=" << strerror(errno);
            return nullptr;
        }
        return rt;
    }

    IPv6Address::IPv6Address()  //初始化sockaddr_in6的IP地址
    {
        memset(&m_addr,0,sizeof(m_addr));   //清零————将m_addr中当前位置后面的sizeof(m_addr)个字节用0替换并返回m_addr
        m_addr.sin6_family=AF_INET6;  //sin6_family协议族  AF_INET6 IPv6
    }

    IPv6Address::IPv6Address(const sockaddr_in6& address)   //通过sockaddr_in6构造IPv6Address,初始化sockaddr_in6
    {
        m_addr=address;
    }

    IPv6Address::IPv6Address(const uint8_t address[16], uint16_t port)   //初始化sockaddr_in6，通过二进制地址构造IPv6Address
    {
        memset(&m_addr,0,sizeof(m_addr));   //清零————将m_addr中当前位置后面的sizeof(m_addr)个字节用0替换并返回m_addr
        m_addr.sin6_family=AF_INET6;  //sin6_family协议族  AF_INET6 IPv6
        m_addr.sin6_port=byteswapOnLittleEndian(port);   //字节序转换    sin6_port端口号
        memcpy(&m_addr.sin6_addr.s6_addr,address,16);   //复制address前16个字节到m_addr.sin6_addr.s6_addr中，返回一个指向目标存储区m_addr.sin6_addr.s6_addr的指针
    }

    const sockaddr* IPv6Address::getAddr() const    //返回sockaddr指针,只读
    {
        return (sockaddr*)&m_addr;
    }
    
    sockaddr* IPv6Address::getAddr()    //返回sockaddr指针,读写
    {
        return (sockaddr*)&m_addr;
    }
    
    socklen_t IPv6Address::getAddrLen() const   //返回sockaddr的长度
    {
        return sizeof(m_addr);
    }

    std::ostream& IPv6Address::insert(std::ostream& os) const   //流式输出地址、端口号
    {
        os<<"[";
        uint16_t* addr=(uint16_t*)m_addr.sin6_addr.s6_addr; //取出IPv6地址
        bool used_zeros=false;  //标记已出现 非0
        for (size_t i = 0; i < 8; ++i)  //将IPv6地址存进流输出([xx:xx:xx:xx:xx:xx:xx:xx]:端口号)
        {   //  0:0:0:0:xx:xx:xx:xx ===> ::xx:xx:xx:xx
            if(addr[i]==0 && !used_zeros)
            {
                continue;
            }
            if(i && addr[i-1]==0 && !used_zeros)
            {
                os<<":";
                used_zeros=true;    //开始出现 非0
            }
            if(i)
            {
                os<<":";
            }
            os<<std::hex<<(int)byteswapOnLittleEndian(addr[i])<<std::dec;   //std::hex<<  <<std::dec    已十六进制输出
        }
        if(!used_zeros && addr[7]==0)   //  0:0:0:0:0:0:0:0 ===> ::
        {
            os<<"::";
        }
        os<<"]:"<<byteswapOnLittleEndian(m_addr.sin6_port); //将端口号存进流输出
        return os;
    }

    IPAddress::ptr IPv6Address::broadcastAddress(uint32_t prefix_len)   //获取该地址的广播地址
    {
        sockaddr_in6 baddr(m_addr);
        baddr.sin6_addr.s6_addr[prefix_len / 8] |= CreateMask<uint8_t>(prefix_len % 8); //计算好所在的8位的广播地址(网络地址中的网络地址部分不变，主机地址变为全1，结果就是广播地址。)
        for(int i = prefix_len / 8 + 1; i < 16; ++i)    //8*16=128 网络地址不变，主机地址全为1
        {
            baddr.sin6_addr.s6_addr[i] = 0xff;  //0xff 255
        }
        return IPv6Address::ptr(new IPv6Address(baddr));
    }

    IPAddress::ptr IPv6Address::networdAddress(uint32_t prefix_len) //获取该地址的网段(IP地址不变=任意一个IP地址)
    {
        sockaddr_in6 baddr(m_addr);
        baddr.sin6_addr.s6_addr[prefix_len / 8] &= CreateMask<uint8_t>(prefix_len % 8); //计算好所在的8位的网段
        for(int i = prefix_len / 8 + 1; i < 16; ++i)    //8*16=128 网络地址不变，主机地址全为0
        {
            baddr.sin6_addr.s6_addr[i] = 0x00;  //0x00 0
        }
        return IPv6Address::ptr(new IPv6Address(baddr));
    }

    IPAddress::ptr IPv6Address::subnetMask(uint32_t prefix_len) //获取子网掩码地址
    {
        sockaddr_in6 subnet;
        memset(&subnet, 0, sizeof(subnet));
        subnet.sin6_family = AF_INET6;
        subnet.sin6_addr.s6_addr[prefix_len /8] = ~CreateMask<uint8_t>(prefix_len % 8); //计算好所在的8位的子网掩码——主机地址全为0
        
        for(uint32_t i = 0; i < prefix_len / 8; ++i)    //网络地址全为1 主机地址全为0
        {
            subnet.sin6_addr.s6_addr[i] = 0xff; //0xff 255
        }
        return IPv6Address::ptr(new IPv6Address(subnet));
    }

    uint32_t IPv6Address::getPort() const   //返回端口号
    {
        return byteswapOnLittleEndian(m_addr.sin6_port);
    }
    
    void IPv6Address::setPort(uint16_t v)   //设置端口号
    {
        m_addr.sin6_port=byteswapOnLittleEndian(v);
    }

    //UnixAddress
    static const size_t MAX_PATH_LEN = sizeof(((sockaddr_un*)0)->sun_path) - 1; //取出sun_path的最大长度    -1 是'\0'
    
    UnixAddress::UnixAddress()  //初始化sockaddr_un的IP地址,长度
    {
        memset(&m_addr,0,sizeof(m_addr));   //清零————将m_addr中当前位置后面的sizeof(m_addr)个字节用0替换并返回m_addr
        m_addr.sun_family=AF_UNIX;  //sun_family协议族  AF_UNIX Unix
        m_length=offsetof(sockaddr_un,sun_path) + MAX_PATH_LEN; //offsetof获取结构体中某个成员相对于结构体起始地址的偏移量。
    }

    UnixAddress::UnixAddress(const std::string& path)   //通过路径构造UnixAddress
    {
        memset(&m_addr,0,sizeof(m_addr));   //清零————将m_addr中当前位置后面的sizeof(m_addr)个字节用0替换并返回m_addr
        m_addr.sun_family=AF_UNIX;  //sun_family协议族  AF_UNIX Unix
        m_length=path.size()+1; //真实路径占有地址

        if(!path.empty() && path[0]=='\0')  //path为空
        {
            --m_length;
        }

        if(m_length > sizeof(m_addr.sun_path))  //超出最大长度
        {
            throw std::logic_error("path too long");
        }
        memcpy(m_addr.sun_path, path.c_str(), m_length);    //复制path.c_str()前m_length个字节到m_addr.sun_path中，返回一个指向目标存储区m_addr.sun_path的指针
        m_length += offsetof(sockaddr_un, sun_path);    //sockaddr地址结构体长度=路径之前结构体的长度+真实路径占有地址
                                                        //(offsetof获取结构体中某个成员相对于结构体起始地址的偏移量。)
    }

    const sockaddr* UnixAddress::getAddr() const    //返回sockaddr指针,只读
    {
        return (sockaddr*)&m_addr;
    }
    
    sockaddr* UnixAddress::getAddr()    //返回sockaddr指针,读写
    {
        return (sockaddr*)&m_addr;
    }
    
    socklen_t UnixAddress::getAddrLen() const   //返回sockaddr的长度
    {
        return m_length;
    }

    std::ostream& UnixAddress::insert(std::ostream& os) const   //流式输出socket类型的文件在文件系统中的路径
    {
        if(m_length > offsetof(sockaddr_un, sun_path)   //offsetof获取结构体中某个成员相对于结构体起始地址的偏移量
                && m_addr.sun_path[0] == '\0')  //sun_path不存在
        {
            return os << "\\0" << std::string(m_addr.sun_path + 1,
                    m_length - offsetof(sockaddr_un, sun_path) - 1);    //以m_addr.sun_path + 1为起始位置的m_length - offsetof(sockaddr_un, sun_path) - 1个字符
        }
        return os << m_addr.sun_path;
    }

    void UnixAddress::setAddrLen(uint32_t v)    //设置sockaddr的长度
    {
        m_length=v;
    }

    std::string UnixAddress::getPath() const    //返回路径
    {
        std::stringstream ss;
        if(m_length > offsetof(sockaddr_un, sun_path)   //offsetof获取结构体中某个成员相对于结构体起始地址的偏移量
                && m_addr.sun_path[0] == '\0')  //sun_path不存在
        {
            ss << "\\0" << std::string(m_addr.sun_path + 1,
                    m_length - offsetof(sockaddr_un, sun_path) - 1);    //以m_addr.sun_path + 1为起始位置的m_length - offsetof(sockaddr_un, sun_path) - 1个字符
        }
        else
        {
            ss << m_addr.sun_path;
        }
        return ss.str();
    }

    //UnknownAddress
    UnknownAddress::UnknownAddress(int family)  //初始化sockaddr_un的IP地址
    {
        memset(&m_addr,0,sizeof(m_addr));   //清零————将m_addr中当前位置后面的sizeof(m_addr)个字节用0替换并返回m_addr
        m_addr.sa_family=family;    //sa_family协议族
    }

    UnknownAddress::UnknownAddress(const sockaddr& addr)    //通过sockaddr构造UnknownAddress,初始化sockaddr
    {
        m_addr=addr;
    }

    const sockaddr* UnknownAddress::getAddr() const    //返回sockaddr指针,只读
    {
        return &m_addr;
    }
    
    sockaddr* UnknownAddress::getAddr()    //返回sockaddr指针,读写
    {
        return &m_addr;
    }
    
    socklen_t UnknownAddress::getAddrLen() const   //返回sockaddr的长度
    {
        return sizeof(m_addr);
    }

    std::ostream& UnknownAddress::insert(std::ostream& os) const   //流式输出地址、端口号
    {
        os << "[UnknownAddress family=" << m_addr.sa_family << "]";
        return os;
    }

    //全局
    std::ostream& operator<<(std::ostream& os, const Address& addr) //流式输出Address
    {
        return addr.insert(os);
    }
}