#include<iostream>
#include"../sylar_server/log.h"
#include"../sylar_server/util.h"
using namespace std;


int main(int argc,char ** argv)
{
    sylar::Logger::ptr logger(new sylar::Logger);
    logger->addAppender(sylar::LogAppender::ptr(new sylar::StdoutLogAppender)); //给输出目标定义输出到控制台的目标

    sylar::FileLogAppender::ptr file_appender(new sylar::FileLogAppender("./log.txt"));
    sylar::LogFormatter::ptr fmt(new sylar::LogFormatter("%d%T%p%T%m%n"));
    file_appender->setFormatter(fmt);//将默认格式器的格式更改
    file_appender->setLevel(sylar::LogLevel::ERROR);

    logger->addAppender(file_appender);//给输出目标定义输出到log.txt文件的目标

    // sylar::LogEvent::ptr event(new sylar::LogEvent(__FILE__,__LINE__,0,sylar::GetThreadId(),sylar::GetFiberId(),time(0)));  //日志事件的数据
    // event->getSS()<<"hello sylar log";

    // logger->log(sylar::LogLevel::DEBUG,event);  //将数据给日志器
    std::cout<<"hello sylar log"<<std::endl;

    SYLAR_LOG_INFO(logger)<<"test macro";
    SYLAR_LOG_ERROR(logger)<<"test error";

    SYLAR_LOG_FMT_ERROR(logger,"test macro fmt error %s","aa");

    auto l=sylar::LoggerMgr::GetInstance()->getLogger("XX"); 
    SYLAR_LOG_INFO(l)<<"XXX";
    return 0;
}
