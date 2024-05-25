#include"../sylar_server/sylar_server.h"

sylar::Logger::ptr g_logger=SYLAR_LOG_ROOT();

int count=0;    //公共数据
//sylar::RWMutex s_mutex; //读写互斥量
sylar::Mutex s_mutex; //互斥量


void fun1()
{
    SYLAR_LOG_INFO(g_logger)<<"name: "<<sylar::Thread::GetName()
                            <<" this.name: "<<sylar::Thread::GetThis()->getName()
                            <<" id: "<<sylar::GetThreadId()
                            <<" this.id: "<<sylar::Thread::GetThis()->getId();
    for(int i=0;i<10000;++i)
    {
        //sylar::RWMutex::WriteLock lock(s_mutex);    //加写锁，超出作用域的加锁————lock是对象
        sylar::Mutex::Lock lock(s_mutex);    //加写锁，超出作用域的加锁 ————lock是对象
        
        ++count;
    }
}

void fun2()
{
    while(true)
    {
        SYLAR_LOG_INFO(g_logger)<<"*************************";
    }
}

void fun3()
{
    while (true)
    {
        SYLAR_LOG_INFO(g_logger)<<"=========================";
    }
}

int main(int argc,char** argv)
{
    SYLAR_LOG_INFO(g_logger)<<"thread test begin";

    YAML::Node root=YAML::LoadFile("/home/yangxin/sylar-master/bin/conf/log2.yml");  //LoadFile（）将yaml文件生成Node形式
    sylar::Config::LoadFromYaml(root);

    std::vector<sylar::Thread::ptr> thrs;
    for(int i=0;i<2;++i)   //创建2个线程
    {
        sylar::Thread::ptr thr(new sylar::Thread(&fun2,"name_"+std::to_string(i*2)));     //cb函数指针指向 fun1函数   to_string将整数i转换为字符串表示形式
        sylar::Thread::ptr thr2(new sylar::Thread(&fun3,"name_"+std::to_string(i*2+1)));     //cb函数指针指向 fun1函数   to_string将整数i转换为字符串表示形式
        thrs.push_back(thr);
        thrs.push_back(thr2);
    }

    
    for(size_t i=0;i<thrs.size();++i)
    {
        thrs[i]->join();    //等待子线程执行完
    }
    SYLAR_LOG_INFO(g_logger)<<"thread test end";
    SYLAR_LOG_INFO(g_logger)<<"count= "<<count;

    return 0;
}