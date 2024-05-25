#include"../sylar_server/hook.h"
#include"../sylar_server/iomanager.h"
#include"../sylar_server/log.h"
#include<sys/socket.h>
#include<sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

sylar::Logger::ptr g_logger=SYLAR_LOG_ROOT();

void test_sleep()   //三秒内输出三个日志，说明sleep没有独占线程时间
{
    sylar::IOManager iom(1);
    iom.schedule([](){
        sleep(2);
        SYLAR_LOG_INFO(g_logger)<<"sleep 2";
    });

    iom.schedule([](){
        sleep(3);
        SYLAR_LOG_INFO(g_logger)<<"sleep 3";
    });
    SYLAR_LOG_INFO(g_logger)<<"test_sleep";
}   

void test_sock()
{
    int sock=socket(AF_INET,SOCK_STREAM,0); //创建socket

    sockaddr_in addr;
    memset(&addr,0,sizeof(addr));   //清零————将addr中当前位置后面的sizeof(addr)个字节用0替换并返回addr
    addr.sin_family=AF_INET;
    addr.sin_port=htons(80);
    inet_pton(AF_INET,"120.232.145.185",&addr.sin_addr.s_addr); //将点分文本的IP地址转换为二进制网络字节序”的IP地址

    SYLAR_LOG_INFO(g_logger) << "begin connect";
    int rt=connect(sock,(const sockaddr*)&addr,sizeof(addr));   //套接字与指定的IP地址绑定====>bind
    SYLAR_LOG_INFO(g_logger) << "connect rt=" << rt << " errno=" << errno;

    if(rt)  //绑定失败
    {
        return;
    }

    const char data[] = "GET / HTTP/1.0\r\n\r\n";
    rt = send(sock, data, sizeof(data), 0); //发送数据
    SYLAR_LOG_INFO(g_logger) << "send rt=" << rt << " errno=" << errno;

    if(rt <= 0) //发送失败
    {
        return;
    }

    std::string buff;
    buff.resize(4096);

    rt = recv(sock, &buff[0], buff.size(), 0);  //接受数据
    SYLAR_LOG_INFO(g_logger) << "recv rt=" << rt << " errno=" << errno;

    if(rt <= 0)
    {
        return;
    }

    buff.resize(rt);
    SYLAR_LOG_INFO(g_logger) << buff;
}

int main(int argc,char** argv)
{
    //test_sock();
    //test_sleep();
    sylar::IOManager iom;
    iom.schedule(test_sock);    //启用hook
    return 0;
}