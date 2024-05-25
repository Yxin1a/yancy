//客户端连接百度服务器
#include"../sylar_server/socket.h"
#include"../sylar_server/sylar_server.h"
#include"../sylar_server/iomanager.h"

static sylar::Logger::ptr g_logger=SYLAR_LOG_ROOT();

void test_socket()
{
    sylar::IPAddress::ptr addr=sylar::Address::LookupAnyIPAddress("www.baidu.com"); //域名返回对应条件的任意IPAddress
    if(addr)
    {
        SYLAR_LOG_INFO(g_logger)<<"get address:"<<addr->toString();
    }
    else
    {
        SYLAR_LOG_INFO(g_logger)<<"get address fail";
        return;
    }
    sylar::Socket::ptr sock=sylar::Socket::CreateTCP(addr); //创建TCP Socket(满足地址类型)
    addr->setPort(80);
    
    if (!sock->connect(addr))
    {
        SYLAR_LOG_INFO(g_logger)<<"connect "<<addr->toString()<<" fail";
        return;
    }
    else
    {
        SYLAR_LOG_INFO(g_logger)<<"connect "<<addr->toString()<<" fail";    //连接成功
    }
    const char buff[]="GET / HTTP/1.0\r\n\r\n";
    int rt=sock->send(buff,sizeof(buff));   //发送请求
    if(rt<=0)
    {
        SYLAR_LOG_INFO(g_logger)<<"send fail rt="<<rt;
        return;
    }

    std::string buffs;
    buffs.resize(4096);
    rt=sock->recv(&buffs[0],buffs.size());  //接收数据

    if(rt<=0)
    {
        SYLAR_LOG_INFO(g_logger)<<"recv fail rt="<<rt;
        return;
    }

    buffs.resize(rt);
    SYLAR_LOG_INFO(g_logger)<<buffs;
}

int main(int argc,char** argv)
{
    sylar::IOManager iom;
    iom.schedule(&test_socket);
    return 0;
}