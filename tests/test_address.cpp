#include"../sylar_server/address.h"
#include"../sylar_server/log.h"

sylar::Logger::ptr g_logger=SYLAR_LOG_ROOT();

void test() //测试域名
{
    std::vector<sylar::Address::ptr> addrs;

    SYLAR_LOG_INFO(g_logger)<<"begin";
    bool v=sylar::Address::Lookup(addrs,"www.sylar.top");   //查看域名的address
    SYLAR_LOG_INFO(g_logger)<<"end";
    if(!v)
    {
        SYLAR_LOG_ERROR(g_logger)<<"lookup fail";
        return;
    }

    for(size_t i=0;i<addrs.size();++i)
    {
        SYLAR_LOG_INFO(g_logger)<<i<<"-"<<addrs[i]->toString(); //返回可读性字符串
    }
}

void test_iface()   //测试网卡
{
    std::multimap<std::string,std::pair<sylar::Address::ptr,uint32_t>> results;

    bool v=sylar::Address::GetInterfaceAddresses(results);  //返回本机所有网卡的
    if(!v)
    {
        SYLAR_LOG_ERROR(g_logger)<<"GetInterfaceAddresses fail";
        return;
    }

    for(auto& i:results)
    {
        SYLAR_LOG_INFO(g_logger)<<i.first<<"-"<<i.second.first->toString()<<"-" //返回可读性字符串
            <<i.second.second;
    }
}

void test_ipv4()    //测试ipv4
{
    auto addr=sylar::IPAddress::Create("www.baidu.com");
    if(addr)
    {
        SYLAR_LOG_INFO(g_logger)<<addr->toString();
    }
}

int main(int argc,char** argv)
{
    test_ipv4();
    test();
    test_iface();
    return 0;
}