#include"../sylar_server/tcp_server.h"
#include"../sylar_server/log.h"
#include"../sylar_server/iomanager.h"
#include"../sylar_server/bytearray.h"
#include"../sylar_server/address.h"

static sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

class EchoServer : public sylar::TcpServer  //TCP例子
{
public:
    EchoServer(int type);   
    void handleClient(sylar::Socket::ptr client);   //处理新连接的Socket类(处理新连接的客户端数据传输)

private:
    int m_type = 0; //2二进制形式，1文本形式
};

EchoServer::EchoServer(int type)    //初始化
    :m_type(type) 
{}

void EchoServer::handleClient(sylar::Socket::ptr client)    //处理新连接的Socket类(处理新连接的客户端数据传输)
{
    SYLAR_LOG_INFO(g_logger) << "handleClient " << *client;   
    sylar::ByteArray::ptr ba(new sylar::ByteArray); //二进制数组,提供基础类型的序列化
    while(true)
    {
        ba->clear();    //清空ByteArray(数据)
        std::vector<iovec> iovs;
        ba->getWriteBuffers(iovs, 1024);    //获取可写入的缓存,保存成iovec数组

        int rt = client->recv(&iovs[0], iovs.size());   //接受数据(TCP) recv函数已进行序列化
        if(rt == 0) //被关闭
        {
            SYLAR_LOG_INFO(g_logger) << "client close: " << *client;
            break;
        }
        else if(rt < 0) //出错
        {
            SYLAR_LOG_INFO(g_logger) << "client error rt=" << rt
                << " errno=" << errno << " errstr=" << strerror(errno);
            break;
        }
        ba->setPosition(ba->getPosition() + rt);    //重新设置操作位置
        ba->setPosition(0);
        //SYLAR_LOG_INFO(g_logger) << "recv rt=" << rt << " data=" << std::string((char*)iovs[0].iov_base, rt);
        if(m_type == 1) //文本
        {//text 
            std::cout << ba->toString();// << std::endl;
        }
        else    //二进制
        {
            std::cout << ba->toHexString();// << std::endl;
        }
        std::cout.flush();  //强制刷新流
    }
}

int type = 1;

void run()
{
    SYLAR_LOG_INFO(g_logger) << "server type=" << type;
    EchoServer::ptr es(new EchoServer(type));
    auto addr = sylar::Address::LookupAny("0.0.0.0:8020");  //通过host地址返回对应条件的任意Address
    while(!es->bind(addr))  //绑定地址
    {
        sleep(2);
    }
    es->start();    //启动服务器
}

int main(int argc,char** argv)
{
    if(argc < 2)
    {
        SYLAR_LOG_INFO(g_logger) << "used as[" << argv[0] << " -t] or [" << argv[0] << " -b]";
        return 0;
    }

    if(!strcmp(argv[1], "-b"))  //-b是文本形式
    {
        type = 2;
    }

    sylar::IOManager iom(2);
    iom.schedule(run);
    return 0;
}