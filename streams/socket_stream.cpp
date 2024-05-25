#include"socket_stream.h"

namespace sylar
{

    SocketStream::SocketStream(Socket::ptr sock, bool owner)    //初始化
        :m_socket(sock)
        ,m_owner(owner)
    {}

    SocketStream::~SocketStream()   //释放内存
    {
        if(m_owner && m_socket) 
        {
            m_socket->close();
        }
    }

    int SocketStream::read(void* buffer, size_t length) //读取数据(字符串)
    {
        if(!isConnected())
        {
            return -1;
        }
        return m_socket->recv(buffer, length);
    }

    int SocketStream::read(ByteArray::ptr ba, size_t length)    //读取数据(iovec数组)
    {
        if(!isConnected())
        {
            return -1;
        }
        std::vector<iovec> iovs;
        ba->getWriteBuffers(iovs, length);  //获取可写入的缓存,保存成iovec数组
        int rt = m_socket->recv(&iovs[0], iovs.size()); //接受数据(TCP)，并存储为iovs数组
        if(rt > 0)
        {
            ba->setPosition(ba->getPosition() + rt);    //重新设置操作位置
        }
        return rt;
    }

    int SocketStream::write(const void* buffer, size_t length)  //写入数据(字符串)
    {
        if(!isConnected())
        {
            return -1;
        }
        return m_socket->send(buffer, length);
    }

    int SocketStream::write(ByteArray::ptr ba, size_t length)   //写入数据(iovec数组)
    {
        if(!isConnected())
        {
            return -1;
        }
        std::vector<iovec> iovs;
        ba->getReadBuffers(iovs, length);   //获取可读取的缓存,保存成iovec数组
        int rt = m_socket->send(&iovs[0], iovs.size()); //发送数据(TCP)，并存储为iovs数组
        if(rt > 0)
        {
            ba->setPosition(ba->getPosition() + rt);    //重新设置操作位置
        }
        return rt;
    }

    void SocketStream::close()  //关闭socket
    {
        if(m_socket)
        {
            m_socket->close();
        }
    }

    bool SocketStream::isConnected() const  //返回是否连接
    {
        return m_socket && m_socket->isConnected(); //isConnected返回是否连接成功
    }

    Address::ptr SocketStream::getRemoteAddress()   //获取远端地址(本机：服务器——客户器地址     本机：客户器——服务器地址)
    {
        if(m_socket)
        {
            return m_socket->getRemoteAddress();    //获取远端地址(本机：服务器——客户器地址     本机：客户器——服务器地址)
        }
        return nullptr;
    }

    Address::ptr SocketStream::getLocalAddress()    //获取本地地址(本机：服务器——服务器地址     本机：客户器——客户器地址)
    {
        if(m_socket)
        {
            return m_socket->getLocalAddress(); //获取本地地址(本机：服务器——服务器地址     本机：客户器——客户器地址)
        }
        return nullptr;
    }

    std::string SocketStream::getRemoteAddressString()  //返回远端地址的可读性字符串
    {
        auto addr = getRemoteAddress();
        if(addr)
        {
            return addr->toString();
        }
        return "";
    }

    std::string SocketStream::getLocalAddressString()   //返回本地地址的可读性字符串
    {
        auto addr = getLocalAddress();
        if(addr)
        {
            return addr->toString();
        }
        return "";
    }

}