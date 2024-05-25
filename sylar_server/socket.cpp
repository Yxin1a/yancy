#include"socket.h"
#include "iomanager.h"
#include "fd_manager.h"
#include "log.h"
#include "macro.h"
#include "hook.h"
#include <limits.h>

namespace sylar
{
    static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

    //Socket
    Socket::ptr Socket::CreateTCP(sylar::Address::ptr address)  //创建TCP Socket(满足地址类型)
    {
        Socket::ptr sock(new Socket(address->getFamily(), TCP, 0));
        return sock;
    }

    Socket::ptr Socket::CreateUDP(sylar::Address::ptr address)  //创建UDP Socket(满足地址类型)
    {
        Socket::ptr sock(new Socket(address->getFamily(), UDP, 0));
        sock->newSock();    //创建新的socket(赋予到m_sock)
        sock->m_isConnected = true;
        return sock;
    }
    
    Socket::ptr Socket::CreateTCPSocket()   //创建IPv4的TCP Socket
    {
        Socket::ptr sock(new Socket(IPv4, TCP, 0));
        return sock;
    }
    
    Socket::ptr Socket::CreateUDPSocket()   //创建IPv4的UDP Socket
    {
        Socket::ptr sock(new Socket(IPv4, UDP, 0));
        sock->newSock();    //创建新的socket(赋予到m_sock)
        sock->m_isConnected = true;
        return sock;
    }
    
    Socket::ptr Socket::CreateTCPSocket6()  //创建IPv6的TCP Socket
    {
        Socket::ptr sock(new Socket(IPv6, TCP, 0));
        return sock;
    }
    
    Socket::ptr Socket::CreateUDPSocket6()  //创建IPv6的UDP Socket
    {
        Socket::ptr sock(new Socket(IPv6, UDP, 0));
        sock->newSock();    //创建新的socket(赋予到m_sock)
        sock->m_isConnected = true;
        return sock;
    }
    
    Socket::ptr Socket::CreateUnixTCPSocket()   //创建Unix的TCP Socket
    {
        Socket::ptr sock(new Socket(UNIX, TCP, 0));
        return sock;
    }
    
    Socket::ptr Socket::CreateUnixUDPSocket()   //创建Unix的UDP Socket
    {
        Socket::ptr sock(new Socket(UNIX, UDP, 0));
        return sock;
    }
    
    Socket::Socket(int family, int type, int protocol)  //Socket构造函数 初始化
        :m_sock(-1)
        ,m_family(family)
        ,m_type(type)
        ,m_protocol(protocol)
        ,m_isConnected(false)
    {}

    Socket::~Socket()   //清除socket
    {
        close();
    }

    int64_t Socket::getSendTimeout()   //获取发送超时时间(毫秒)
    {
        FdCtx::ptr ctx = FdMgr::GetInstance()->get(m_sock); //获取/创建文件句柄类FdCtx
        if(ctx)
        {
            return ctx->getTimeout(SO_SNDTIMEO);    //获取超时时间  SO_RCVTIMEO(读超时), SO_SNDTIMEO(写超时)
        }
        return -1;
    }
    
    void Socket::setSendTimeout(int64_t v) //设置发送超时时间(毫秒)
    {
        struct timeval tv{int(v / 1000), int(v % 1000 * 1000)}; //设置毫秒
        setOption(SOL_SOCKET, SO_SNDTIMEO, tv); //SOL_SOCKET套接字层次    SO_SNDTIMEO发送超时时间
    }
    
    int64_t Socket::getRecvTimeout()   //获取接受超时时间(毫秒)
    {
        FdCtx::ptr ctx = FdMgr::GetInstance()->get(m_sock); //获取/创建文件句柄类FdCtx
        if(ctx)
        {
            return ctx->getTimeout(SO_RCVTIMEO);    //获取超时时间  SO_RCVTIMEO(读超时), SO_SNDTIMEO(写超时)
        }
        return -1;
    }
    
    void Socket::setRecvTimeout(int64_t v) //设置接受超时时间(毫秒)
    {
        struct timeval tv{int(v / 1000), int(v % 1000 * 1000)}; //设置毫秒
        setOption(SOL_SOCKET, SO_RCVTIMEO, tv); //SOL_SOCKET套接字层次  SO_RCVTIMEO接收超时时间
    }

    bool Socket::getOption(int level, int option, void* result, socklen_t* len) //获取sockopt  getsockopt获取一个套接字的选项
    {
        int rt = getsockopt(m_sock, level, option, result, (socklen_t*)len);    //获取一个套接字的选项
                                                                                //m_sock：要获取的套接字描述符。
                                                                                //level：协议层次。或为特定协议的代码（如IPv4，IPv6，TCP，SCTP），或为通用套接字代码（SOL_SOCKET）。
                                                                                //option：选项名。level对应的选项，一个level对应多个选项，不同选项对应不同功能。
                                                                                //result：指向某个变量的指针，该变量是要获取选项的值。可以是一个结构体，也可以是普通变量
                                                                                //len：optval的长度。
        if(rt)
        {
            SYLAR_LOG_DEBUG(g_logger) << "getOption sock=" << m_sock
                << " level=" << level << " option=" << option
                << " errno=" << errno << " errstr=" << strerror(errno);
            return false;
        }
        return true;
    }

    bool Socket::setOption(int level, int option, const void* result, socklen_t len)    //设置sockopt  setsockopt设置套接字描述符的属性
    {
        if(setsockopt(m_sock, level, option, result, (socklen_t)len))   //设置套接字描述符的属性
                                                                        //m_sock：要设置的套接字描述符。
                                                                        //level：协议层次。或为特定协议的代码（如IPv4，IPv6，TCP，SCTP），或为通用套接字代码（SOL_SOCKET）。
                                                                        //option：选项名。level对应的选项，一个level对应多个选项，不同选项对应不同功能。
                                                                        //result：指向某个变量的指针，该变量是要设置选项的值。可以是一个结构体，也可以是普通变量
                                                                        //len：optval的长度。
        {
            SYLAR_LOG_DEBUG(g_logger) << "setOption sock=" << m_sock
                << " level=" << level << " option=" << option
                << " errno=" << errno << " errstr=" << strerror(errno);
            return false;
        }
        return true;
    }

    Socket::ptr Socket::accept()    //服务器接收connect(客户端)链接，成功返回新连接的socket(客户端)
    {
        Socket::ptr sock(new Socket(m_family, m_type, m_protocol)); //客户端Socket类指针
        int newsock = ::accept(m_sock, nullptr, nullptr);   //newsock客户端对象 ::accept用的是系统的函数(全局)  *accept函数是接受所有和服务器连接的客户端
        if(newsock == -1)   //失败
        {
            SYLAR_LOG_ERROR(g_logger) << "accept(" << m_sock << ") errno="
                << errno << " errstr=" << strerror(errno);
            return nullptr;
        }
        if(sock->init(newsock)) //建立客户端的Socket
        {
            return sock;
        }
        return nullptr;
    }

    bool Socket::bind(const Address::ptr addr)  //服务器绑定IP地址
    {
        if(!isValid())  //m_sock是否有效
        {
            newSock();  //创建新的socket(赋予到m_sock)
            if(SYLAR_UNLIKELY(!isValid()))  
            {
                return false;
            }
        }

        if(SYLAR_UNLIKELY(addr->getFamily() != m_family))   //协议簇(IP协议)不一致
        {
            SYLAR_LOG_ERROR(g_logger) << "bind sock.family("
                << m_family << ") addr.family(" << addr->getFamily()
                << ") not equal, addr=" << addr->toString();
            return false;
        }
        
        // UnixAddress::ptr uaddr = std::dynamic_pointer_cast<UnixAddress>(addr);
        // if(uaddr)
        // {
        //     Socket::ptr sock = Socket::CreateUnixTCPSocket();
        //     if(sock->connect(uaddr))
        //     {
        //         return false;
        //     }
        //     else
        //     {
        //         sylar::FSUtil::Unlink(uaddr->getPath(), true);
        //     }
        // }
        
        //m_localAddress = addr;
        if(::bind(m_sock, addr->getAddr(), addr->getAddrLen())) //服务器绑定IP地址
        {
            SYLAR_LOG_ERROR(g_logger) << "bind error errrno=" << errno
                << " errstr=" << strerror(errno);
            return false;
        }
        getLocalAddress();  //服务器地址
        return true;
    }

    bool Socket::connect(const Address::ptr addr, uint64_t timeout_ms)  //客户端连接服务器地址
    {
        m_remoteAddress = addr; //服务器地址    本机：客户器
        if(!isValid())  //m_sock是否有效
        {
            newSock();  //创建新的socket(赋予到m_sock)
            if(SYLAR_UNLIKELY(!isValid()))  
            {
                return false;
            }
        }

        if(SYLAR_UNLIKELY(addr->getFamily() != m_family))   //协议簇(IP协议)不一致
        {
            SYLAR_LOG_ERROR(g_logger) << "connect sock.family("
                << m_family << ") addr.family(" << addr->getFamily()
                << ") not equal, addr=" << addr->toString();
            return false;
        }

        if(timeout_ms == (uint64_t)-1)  //没有超时时间
        {
            if(::connect(m_sock, addr->getAddr(), addr->getAddrLen()))  //客户端连接服务器地址
            {
                SYLAR_LOG_ERROR(g_logger) << "sock=" << m_sock << " connect(" << addr->toString()
                    << ") error errno=" << errno << " errstr=" << strerror(errno);
                close();
                return false;
            }
        }
        else
        {
            if(::connect_with_timeout(m_sock, addr->getAddr(), addr->getAddrLen(), timeout_ms)) //客户端连接服务器地址(增加了超时变量)
            {
                SYLAR_LOG_ERROR(g_logger) << "sock=" << m_sock << " connect(" << addr->toString()
                    << ") timeout=" << timeout_ms << " error errno="
                    << errno << " errstr=" << strerror(errno);
                close();
                return false;
            }
        }
        m_isConnected = true;
        getRemoteAddress(); //服务器地址
        getLocalAddress();  //客户端地址
        return true;
    }

    bool Socket::reconnect(uint64_t timeout_ms)
    {
        if(!m_remoteAddress)
        {
            SYLAR_LOG_ERROR(g_logger) << "reconnect m_remoteAddress is null";
            return false;
        }
        m_localAddress.reset();
        return connect(m_remoteAddress, timeout_ms);
    }

    bool Socket::listen(int backlog)
    {
        if(!isValid())  //m_sock是否有效
        {
            SYLAR_LOG_ERROR(g_logger) << "listen error sock=-1";
            return false;
        }
        if(::listen(m_sock, backlog))   //监听socket客户端连接
        {
            SYLAR_LOG_ERROR(g_logger) << "listen error errno=" << errno
                << " errstr=" << strerror(errno);
            return false;
        }
        return true;
    }

    bool Socket::close()    //关闭socket
    {
        if(!m_isConnected && m_sock == -1)  //连接失败 && m_sock无效
        {
            return true;
        }
        m_isConnected = false;
        if(m_sock != -1)    //有效
        {
            ::close(m_sock);    //关闭m_sock
            m_sock = -1;
        }
        return false;
    }

    int Socket::send(const void* buffer, size_t length, int flags)  //发送数据TCP(字符串)
    {
        if(isConnected())   //连接成功
        {
            return ::send(m_sock, buffer, length, flags);   //发送数据  (send和write都是发送数据，各有优点)
        }
        return -1;
    }

    int Socket::send(const iovec* buffers, size_t length, int flags)    //发送数据TCP(iovec数组)
    {
        if(isConnected())   //连接成功
        {
            msghdr msg; //存储iovec数组
            memset(&msg, 0, sizeof(msg));   //清零
            msg.msg_iov = (iovec*)buffers;  //数据 
            msg.msg_iovlen = length;        //数据长度
            return ::sendmsg(m_sock, &msg, flags);  //发送数据(iovec数组)
        }
        return -1;
    }

    int Socket::sendTo(const void* buffer, size_t length, const Address::ptr to, int flags) //发送数据UDP(字符串)
    {
        if(isConnected())   //连接成功
        {
            return ::sendto(m_sock, buffer, length, flags, to->getAddr(), to->getAddrLen());    //指定目的地发送数据，SendTo()适用于发送未建立连接的UDP数据包
        }
        return -1;
    }

    int Socket::sendTo(const iovec* buffers, size_t length, const Address::ptr to, int flags)   //发送数据UDP(iovec数组)
    {
        if(isConnected())   //连接成功
        {
            msghdr msg; //存储iovec数组
            memset(&msg, 0, sizeof(msg));   //清零
            msg.msg_iov = (iovec*)buffers;  //数据
            msg.msg_iovlen = length;    //数据长度
            msg.msg_name = to->getAddr();   //地址结构sockaddr
            msg.msg_namelen = to->getAddrLen(); //地址结构sockaddr长度
            return ::sendmsg(m_sock, &msg, flags);  //发送数据(iovec数组)
        }
        return -1;
    }

    int Socket::recv(void* buffer, size_t length, int flags)    //接收数据TCP(字符串)
    {
        if(isConnected())   //连接成功
        {
            return ::recv(m_sock, buffer, length, flags);   //接收数据  (recv和read都是发送数据，各有优点)
        }
        return -1;
    }

    int Socket::recv(iovec* buffers, size_t length, int flags)  //接收数据TCP(iovec数组)
    {
        if(isConnected())   //连接成功
        {
            msghdr msg; //存储iovec数组
            memset(&msg, 0, sizeof(msg));   //清零
            msg.msg_iov = (iovec*)buffers;  //数据
            msg.msg_iovlen = length;    //数据长度
            return ::recvmsg(m_sock, &msg, flags);  //接收数据(iovec数组)
        }
        return -1;
    }

    int Socket::recvFrom(void* buffer, size_t length, Address::ptr from, int flags) //接收数据UDP(字符串)
    {
        if(isConnected())   //连接成功
        {
            socklen_t len = from->getAddrLen();
            return ::recvfrom(m_sock, buffer, length, flags, from->getAddr(), &len);    //指定发送端接收数据，recvfrom()适用于发送未建立连接的UDP数据包
        }
        return -1;
    }

    int Socket::recvFrom(iovec* buffers, size_t length, Address::ptr from, int flags)   //接收数据UDP(iovec数组)
    {
        if(isConnected())   //连接成功
        {
            msghdr msg; //存储iovec数组
            memset(&msg, 0, sizeof(msg));   //清零
            msg.msg_iov = (iovec*)buffers;  //数据
            msg.msg_iovlen = length;    //数据长度
            msg.msg_name = from->getAddr();     //地址结构sockaddr
            msg.msg_namelen = from->getAddrLen();   //地址结构sockaddr长度
            return ::recvmsg(m_sock, &msg, flags);  //接收数据(iovec数组)
        }
        return -1;
    }
    
    Address::ptr Socket::getRemoteAddress()    //获取远端地址(本机：服务器——客户器地址     本机：客户器——服务器地址)
    {
        if(m_remoteAddress) //存在
        {
            return m_remoteAddress;
        }

        Address::ptr result;
        switch(m_family)
        {
            case AF_INET:   //IPv4
                result.reset(new IPv4Address());
                break;
            case AF_INET6:  //IPv6
                result.reset(new IPv6Address());
                break;
            case AF_UNIX:   //Unix
                result.reset(new UnixAddress());
                break;
            default:        //未知
                result.reset(new UnknownAddress(m_family));
                break;
        }
        socklen_t addrlen = result->getAddrLen();
        if(getpeername(m_sock, result->getAddr(), &addrlen))    //getpeername获取与m_sock套接字关联的外地协议地址  赋予到result(sockaddr结构中)
        {
            return Address::ptr(new UnknownAddress(m_family));  //getpeername失败
        }
        if(m_family == AF_UNIX) //Unix
        {
            UnixAddress::ptr addr = std::dynamic_pointer_cast<UnixAddress>(result); //dynamic_pointer_cast父类类型转化子类类型
            addr->setAddrLen(addrlen);
        }
        m_remoteAddress = result;
        return m_remoteAddress;
    }
    
    Address::ptr Socket::getLocalAddress() //获取本地地址(本机：服务器——服务器地址     本机：客户器——客户器地址)
    {
        if(m_localAddress)  //存在
        {
            return m_localAddress;
        }

        Address::ptr result;
        switch(m_family)
        {
            case AF_INET:   //IPv4
                result.reset(new IPv4Address());
                break;
            case AF_INET6:  //IPv6
                result.reset(new IPv6Address());
                break;
            case AF_UNIX:   //Unix
                result.reset(new UnixAddress());
                break;
            default:    //未知
                result.reset(new UnknownAddress(m_family));
                break;
        }
        socklen_t addrlen = result->getAddrLen();
        if(getsockname(m_sock, result->getAddr(), &addrlen))    //getsockname获取与m_sock套接字关联的本地协议地址  赋予到result(sockaddr结构中)
        {
            SYLAR_LOG_ERROR(g_logger) << "getsockname error sock=" << m_sock
                << " errno=" << errno << " errstr=" << strerror(errno);
            return Address::ptr(new UnknownAddress(m_family));  //getsockname失败
        }
        if(m_family == AF_UNIX) //是Unix
        {
            UnixAddress::ptr addr = std::dynamic_pointer_cast<UnixAddress>(result); //dynamic_pointer_cast父类类型转化子类类型
            addr->setAddrLen(addrlen);
        }
        m_localAddress = result;
        return m_localAddress;
    }
    
    bool Socket::isValid() const   //m_sock是否有效(m_sock != -1)
    {
        return m_sock!=-1;
    }
    
    int Socket::getError() //返回Socket的错误
    {
        int error = 0;
        socklen_t len = sizeof(error);
        if(!getOption(SOL_SOCKET, SO_ERROR, &error, &len))  //获取一个套接字的选项  SOL_SOCKET 套接字层次   SO_ERROR获取错误信息
        {
            error = errno;
        }
        return error;
    }
    
    std::ostream& Socket::dump(std::ostream& os) const //输出socket基本信息到流中
    {
        os  << "[Socket sock=" << m_sock
            << " is_connected=" << m_isConnected
            << " family=" << m_family
            << " type=" << m_type
            << " protocol=" << m_protocol;
        if(m_localAddress) {
            os << " local_address=" << m_localAddress->toString();
        }
        if(m_remoteAddress) {
            os << " remote_address=" << m_remoteAddress->toString();
        }
        os << "]";
        return os;
    }

    std::string Socket::toString() const //socket基本信息字符串化
    {
        std::stringstream ss;
        dump(ss);
        return ss.str();
    }

    bool Socket::cancelRead()  //取消读(唤醒执行)
    {
        return IOManager::GetThis()->cancelEvent(m_sock, sylar::IOManager::READ);   //回调函数默认为空
    }
    
    bool Socket::cancelWrite() //取消写(唤醒执行)
    {
        return IOManager::GetThis()->cancelEvent(m_sock, sylar::IOManager::WRITE);  //回调函数默认为空
    }
    
    bool Socket::cancelAccept()    //取消accept(唤醒执行)
    {
        return IOManager::GetThis()->cancelEvent(m_sock, sylar::IOManager::READ);   //回调函数默认为空
    }
    
    bool Socket::cancelAll()   //取消所有事件(唤醒执行)
    {
        return IOManager::GetThis()->cancelAll(m_sock); //回调函数默认为空
    }
    
    void Socket::initSock()    //初始化socket(m_sock)
    {
        int val = 1;
        setOption(SOL_SOCKET, SO_REUSEADDR, val);   //SOL_SOCKET 套接字层次 SO_REUSEADDR是否允许重复使用本地地址
        if(m_type == SOCK_STREAM)   //SOCK_STREAM流格式套接TCP字也叫“面向连接的套接字TCP”
        {
            setOption(IPPROTO_TCP,TCP_NODELAY,val); //IPPROTO_TCP TCP层次   TCP_NODELAY不使用Nagle算法
        }
    }
    
    void Socket::newSock() //创建新的socket(赋予到m_sock)
    {
        m_sock = socket(m_family, m_type, m_protocol);
        if(SYLAR_LIKELY(m_sock != -1))  //成功几率大，(使用可以提高性能)
        {
            initSock(); //初始化socket(m_sock)
        }
        else    //成功几率小，(使用可以提高性能)
        {
            SYLAR_LOG_ERROR(g_logger) << "socket(" << m_family
                << ", " << m_type << ", " << m_protocol << ") errno="
                << errno << " errstr=" << strerror(errno);
        }
    }

    bool Socket::init(int sock) //初始化sock(sock赋予到m_sock)    sock 建立Socket的socket句柄
    {
        FdCtx::ptr ctx = FdMgr::GetInstance()->get(sock);   //获取/创建文件句柄类FdCtx
        if(ctx && ctx->isSocket() && !ctx->isClose())   //是socket      没有关闭
        {
            m_sock = sock;
            m_isConnected = true;
            initSock(); //初始化socket相关的
            getLocalAddress();  //获取远端地址(本机：服务器——客户器地址     本机：客户器——服务器地址)
            getRemoteAddress(); //获取本地地址(本机：服务器——服务器地址     本机：客户器——客户器地址)
            return true;
        }
        return false;
    }

    //SSLSocket
    

    //全局
    std::ostream& operator<<(std::ostream& os, const Socket& sock)
    {
        return sock.dump(os);
    }

}