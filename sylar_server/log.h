//  日志模板封装

//  配置模板控制日志模板
//(获取yml文件配置 和 默认日志模板配置，根据变更事件来修改日志模板配置————在变更事件里修改)

#ifndef __SYLAR_LOG_H__
#define __SYLAR_LOG_H__

#include<string>
#include<stdint.h>
#include<memory>
#include<list>
#include<sstream>
#include<fstream>
#include<vector>
#include<stdarg.h>
#include<map>
#include"util.h"
#include"singLeton.h"
#include"thread.h"

//使用流式方式将日志级别level的日志写入到logger
#define SYLAR_LOG_LEVEL(logger,level)\
    if(logger->getLevel()<=level)\
        sylar::LogEventWrap(sylar::LogEvent::ptr(new sylar::LogEvent(logger,level,\
        __FILE__,__LINE__,0,sylar::GetThreadId(),\
            sylar::GetFiberId(),time(0),sylar::Thread::GetName()))).getSS()

//使用流式方式将日志级别debug的日志写入到logger
#define SYLAR_LOG_DEBUG(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::DEBUG)

//使用流式方式将日志级别info的日志写入到logge
#define SYLAR_LOG_INFO(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::INFO)

//使用流式方式将日志级别warn的日志写入到logger
#define SYLAR_LOG_WARN(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::WARN)

//使用流式方式将日志级别error的日志写入到logger
#define SYLAR_LOG_ERROR(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::ERROR)

//使用流式方式将日志级别fatal的日志写入到logger
#define SYLAR_LOG_FATAL(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::FATAL)


//使用格式化方式将日志级别level的日志写入到logger
#define SYLAR_LOG_FMT_LEVEL(logger, level, fmt, ...) \
    if(logger->getLevel() <= level) \
        sylar::LogEventWrap(sylar::LogEvent::ptr(new sylar::LogEvent(logger, level, \
                        __FILE__,__LINE__, 0, sylar::GetThreadId(),\
                sylar::GetFiberId(), time(0),sylar::Thread::GetName()))).getEvent()->format(fmt, __VA_ARGS__)

//使用格式化方式将日志级别debug的日志写入到logger
#define SYLAR_LOG_FMT_DEBUG(logger, fmt, ...) SYLAR_LOG_FMT_LEVEL(logger, sylar::LogLevel::DEBUG, fmt, __VA_ARGS__)

//使用格式化方式将日志级别info的日志写入到logger
#define SYLAR_LOG_FMT_INFO(logger, fmt, ...)  SYLAR_LOG_FMT_LEVEL(logger, sylar::LogLevel::INFO, fmt, __VA_ARGS__)

//使用格式化方式将日志级别warn的日志写入到logger
#define SYLAR_LOG_FMT_WARN(logger, fmt, ...)  SYLAR_LOG_FMT_LEVEL(logger, sylar::LogLevel::WARN, fmt, __VA_ARGS__)

//使用格式化方式将日志级别error的日志写入到logger
#define SYLAR_LOG_FMT_ERROR(logger, fmt, ...) SYLAR_LOG_FMT_LEVEL(logger, sylar::LogLevel::ERROR, fmt, __VA_ARGS__)

//使用格式化方式将日志级别fatal的日志写入到logger
#define SYLAR_LOG_FMT_FATAL(logger, fmt, ...) SYLAR_LOG_FMT_LEVEL(logger, sylar::LogLevel::FATAL, fmt, __VA_ARGS__)


//获取主日志器
#define SYLAR_LOG_ROOT() sylar::LoggerMgr::GetInstance()->getRoot()
//获取name的日志器
#define SYLAR_LOG_NAME(name) sylar::LoggerMgr::GetInstance()->getLogger(name)


namespace sylar{   //为了区分和别人代码重命名的问题

class Logger;
class LoggerManager;

//日志等级
class LogLevel
{
public:
    //日志级别枚举
    enum  Level
    {
        UNKNOW=0,   // 未知级别
        DEBUG=1,    // DEBUG 级别（调试）
        INFO=2,     // INFO 级别（信息）
        WARN=3,     // WARN 级别（警告）
        ERROR=4,    // ERROR 级别（错误）
        FATAL=5     // FATAL 级别（致命）
    };

    //将日志级别转成文本输出    level 日志级别
    static const char * ToString(LogLevel::Level level);  //返回日志级别

    /**
     *  将文本转换成日志级别
     *  str 日志级别文本
     */
    static LogLevel::Level FromString(const std::string& str);
};

//日志事件
class LogEvent
{
public:
    typedef std::shared_ptr<LogEvent> ptr;   //共享指针
    /**
     * 构造函数
     * logger 日志器
     * level 日志级别
     * file 文件名
     * line 文件行号
     * elapse 程序启动依赖的耗时(毫秒)
     * thread_id 线程id
     * fiber_id 协程id
     * time 日志事件时间(秒)
     * thread_name 线程名称
     */
    LogEvent(std::shared_ptr<Logger>logger,LogLevel::Level level
        ,const char * file,int32_t m_line,uint32_t elapse
        ,uint32_t thread_id,uint32_t fiber_id,uint64_t time
        ,const std::string& thread_name);  //日志事件初始化
    ~LogEvent();

    //返回日志事件的数据
    const char * getFile() const {return m_file;}       //返回文件名
    int32_t getLine() const {return m_line;}            //返回行号
    uint32_t getElapse() const {return m_elapse;}       //返回耗时
    uint32_t getThreadId() const {return m_threadId;}   //返回线程ID
    uint32_t getFiberId() const {return m_fiberID;}     //返回协程ID
    uint64_t getTime() const {return m_time;}           //返回时间
    const std::string& getThreadName() const {return m_threadName;}    //返回线程名称
    std::string getContent() const {return m_ss.str();} //返回日志内容
    std::shared_ptr<Logger> getLogger() const {return m_logger;}//返回日志器
    LogLevel::Level getLevel() const {return m_level;}  //返回日志级别

    std::stringstream & getSS() {return m_ss;}  //返回日志内容字符串流
    void format(const char * fmt,...);          //格式化写入日志内容
    void format(const char* fmt, va_list al);   //格式化写入日志内容

private:
    const char *m_file=nullptr;  //文件名  nullptr相当于null 但更适于C++
    int32_t m_line=0;  //行号  也就是不同平台下，使用以下名称可以保证固定长度。// 1字节 int8_t —— char// 2字节 int16_t —— short// 4字节 int32_t —— int// 8字节 int64_t —— long long
    uint32_t m_elapse=0;    //程序启动开始到现在的毫秒数        //uint32_t-》unsigned int     unsigned-无符号数
    uint32_t m_threadId=0;  //线程ID
    uint32_t m_fiberID=0;   //协程ID
    uint64_t m_time;        //时间戳
    std::string m_threadName;//线程名称
    std::stringstream m_ss; //日志内容流

    std::shared_ptr<Logger> m_logger;   //日志器
    LogLevel::Level m_level;            //日志等级
};

//日志事件包装器
class LogEventWrap
{
public:
    /**
     *  构造函数
     *  e 日志事件
     */
    LogEventWrap(LogEvent::ptr e);  //初始化

    /**
     *  析构函数
     */
    ~LogEventWrap();

    LogEvent::ptr getEvent() const { return m_event;}   //获取日志事件

    std::stringstream & getSS();    //获取日志内容流
private:
    LogEvent::ptr m_event;  //日志事件
};

//日志格式器
class LogFormatter
{
public:
    typedef std::shared_ptr<LogFormatter> ptr;
    /**
     *  构造函数
     * pattern 格式模板
     *
     *  %m 消息
     *  %p 日志级别
     *  %r 累计毫秒数
     *  %c 日志名称
     *  %t 线程id
     *  %n 换行
     *  %d 时间
     *  %f 文件名
     *  %l 行号
     *  %T 制表符
     *  %F 协程id
     *  %N 线程名称
     *
     *  {%Y-%m-%d %H:%M:%S} 时间格式
     * 
     *  默认格式 "%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"
     */
    LogFormatter(const std::string & pattern);  //初始化
    
    //%t %thread_id %m%n
    /**
     *  返回格式化日志文本
     *  logger 日志器
     *  level 日志级别
     *  event 日志事件
     */
    std::string format(std::shared_ptr<Logger> logger,LogLevel::Level level,LogEvent::ptr event);//返回格式化日志文本（字符串）
    std::ostream& format(std::ostream& ofs, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event);    //返回格式化日志文件（文件）

public:

    //日志内容项格式化  负责输出各种格式
    class FormatItem
    {
    public:
        typedef std::shared_ptr<FormatItem> ptr;
        
        virtual ~FormatItem(){} //析构函数

        /**
         *  格式化日志到流
         *  os 日志输出流
         *  logger 日志器
         *  level 日志等级
         *  event 日志事件
         */
        virtual void format(std::ostream & os,std::shared_ptr<Logger> logger,LogLevel::Level level,LogEvent::ptr event)=0;  //返回日志事件
    };

    void init();  //初始化,解析日志模板     Appender的解析

    bool isError() const { return m_error; }    //返回判断日志格式是否有错误
    const std::string getPattern() const {return m_pattern;}    //返回日志模板
private:
    std::string m_pattern;  // 日志格式模板
    std::vector<FormatItem::ptr> m_items;  //日志格式解析后格式     FormarItem的集合   
    bool m_error=false;     //判断日志格式是否有错误
};

//日志输出目标
class LogAppender
{
friend class Logger;    
public:
    typedef std::shared_ptr<LogAppender> ptr;
    typedef Spinlock MutexType;

    //析构函数
    virtual ~LogAppender(){}  //virtual防止内存释放的问题

    /**
     *  写入日志
     *  logger 日志器
     *  level 日志级别
     *  event 日志事件
     */
    virtual void log(std::shared_ptr<Logger> logger,LogLevel::Level level,LogEvent::ptr event)=0;  //打印输出格式

    virtual std::string toYamlString()=0;   //将日志输出目标的配置转成YAML String
    
    void setFormatter(LogFormatter::ptr val); //更改日志格式器   设置输出的格式
    LogFormatter::ptr getFormatter();  //获取日志格式器   返回输出的格式

    LogLevel::Level getLevel() const {return m_level;}  //获取日志级别
    void setLevel(LogLevel::Level val) {m_level=val;}   //设置日志级别
protected:
    LogLevel::Level m_level=LogLevel::DEBUG;        //日志级别(对应的级别)
    bool m_hasFormatter=false;      //判断是否有自己的日志格式器
    MutexType m_mutex;  //互斥量（读写共享数据才要加锁——LogFormatter::ptr共享指针，读写函数）
    LogFormatter::ptr m_formatter;  //日志格式器    定义输出的格式
};

//日志器
class Logger:public std::enable_shared_from_this<Logger>//enable_shared_from_this<>成员函数可以获取自身类的智能指针,Time只能以智能指针形式存在
{

    friend class LoggerManager;

public:

    typedef std::shared_ptr<Logger> ptr;
    typedef Spinlock MutexType;

    /**
     *  构造函数
     *  name 日志器名称
     */
    Logger(const std::string &name="root");

    /**
     *  写日志
     *  level 日志级别
     *  event 日志事件
     */
    void log(LogLevel::Level level,LogEvent::ptr event);  //将日志数据在日志输出地按格式输出
    
    //对日志级别的处理
    void debug(LogEvent::ptr event);    //写debug级别日志   event 日志事件
    void info(LogEvent::ptr event);     //写info级别日志    event 日志事件
    void warn(LogEvent::ptr event);     //写warn级别日志    event 日志事件
    void error(LogEvent::ptr event);    //写error级别日志   event 日志事件
    void fatal(LogEvent::ptr event);    //写fatal级别日志   event 日志事件

    void addAppender(LogAppender::ptr appender);        //Appender（日志输出目标）集合的插入    appender 日志目标
    void delAppender(LogAppender::ptr appender);        //Appender（日志输出目标）集合的删除    appender 日志目标
    void clearAppenders();                              //清空日志目标
    LogLevel::Level getLevel() const{return m_level;};  //返回日志级别
    void setLevel(LogLevel::Level val){m_level=val;};   //设置日志级别（默认限定）

    const std::string & getName() const {return m_name;}  //返会日志名称

    void setFormatter(LogFormatter::ptr val);           //设置日志格式器
    void setFormatter(const std::string& val);          //设置日志格式模板
    LogFormatter::ptr getFormatter();                   //获取日志格式器

    std::string toYamlString();     //将日志器的配置转成YAML String
private:
    std::string m_name;                     //日志名称
    LogLevel::Level m_level;                //日志级别     默认限定级别，超出写日志
    MutexType m_mutex;                      //互斥量（读写共享数据才要加锁——读写函数—LogFormatter::ptr共享指针——Logger::ptr共享指针，<LogAppender::ptr>）
    std::list<LogAppender::ptr> m_appender; //Appender（日志目标）集合
    LogFormatter::ptr m_formatter;          //日志格式器指针     默认日志格式器指针
    Logger::ptr m_root;                     //主日志
};

//输出到控制台的Appender（日志目标集合）
class StdoutLogAppender:public LogAppender
{  
public:
    typedef std::shared_ptr<StdoutLogAppender> ptr;

    void log(Logger::ptr logger,LogLevel::Level level,LogEvent::ptr event) override;  //覆写（override）虚函数,不用virtual

    std::string toYamlString() override;    //将日志输出目标的配置转成YAML String
};

//输出到文件的Appender（日志目标集合）
class FileLogAppender:public LogAppender
{
public:
    typedef std::shared_ptr<FileLogAppender> ptr;
    FileLogAppender(const std::string & filename);
    void log(Logger::ptr logger,LogLevel::Level level,LogEvent::ptr event) override;  //覆写（override）虚函数,不用virtual
    
    std::string toYamlString() override;    //将日志输出目标的配置转成YAML String

    bool reopen();  //重新打开日志文件,文件打开成功返回true
private:
    std::string m_filename;// 输出到文件的路径
    std::ofstream m_filestream;// 文件流
    uint64_t m_lastTime = 0;    // 上次重新打开时间
};

//日志管理器————并与配置系统的整合(当logger的appenders为空时，使用root写logger)
class LoggerManager
{
public:
    typedef Spinlock MutexType;

    LoggerManager();    //构造函数

    Logger::ptr getLogger(const std::string &name); //获取日志器(没有则创建日志器)   name 日志器名称 

    void init();   //初始化,解析日志模板

    Logger::ptr getRoot() const {return m_root;}    //返回主日志器

    std::string toYamlString(); //将所有的日志器配置转成YAML String
private: 
    std::map<std::string,Logger::ptr> m_loggers;    //日志器容器
    Logger::ptr m_root;     // 主日志器
    MutexType m_mutex;  //互斥量（读写共享数据才要加锁——读写函数——Logger::ptr共享指针————<std::string,Logger::ptr>共享指针
};

// 日志器管理类单例模式
typedef sylar::Singleton<LoggerManager> LoggerMgr;

}
#endif
