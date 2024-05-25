#include"log.h"
#include<iostream>
#include<map>
#include<functional>
#include<time.h>
#include<string.h>
#include"util.h"
#include"config.h"
#include<malloc.h>


namespace sylar
{
    
    const char * LogLevel::ToString(LogLevel::Level level)  //将日志级别转成文本输出    返回日志级别
    {//判断日志级别，并返回字符串
        switch (level)
        {//要看
    #define XX(name) \  
        case LogLevel::name: \
            return #name; \
            break;
        //#name,把name变成字符串
        XX(DEBUG);
        XX(INFO);
        XX(WARN);
        XX(ERROR);
        XX(FATAL);
    #undef XX
        default:
            return "UNKNOW";
        }
        return "UNKNOW";
    }

    LogLevel::Level LogLevel::FromString(const std::string& str)    //将文本（string）转换成日志级别
    {
    #define XX(level,v) \
        if(str==#v) \
        { \
            return LogLevel::level; \
        }
        XX(DEBUG,debug);
        XX(INFO,info);
        XX(WARN,warn);
        XX(ERROR,error);
        XX(FATAL,fatal);

        XX(DEBUG,DEBUG);
        XX(INFO,INFO);
        XX(WARN,WARN);
        XX(ERROR,ERROR);
        XX(FATAL,FATAL);
        return LogLevel::UNKNOW;
    #undef XX
    }

    void LogEvent::format(const char* fmt, ...)     //格式化写入日志内容
    {
        va_list al;
        va_start(al, fmt);
        format(fmt, al);
        va_end(al);
    }

    void LogEvent::format(const char* fmt, va_list al)  //格式化写入日志内容
    {
        char* buf = nullptr;
        int len = vasprintf(&buf, fmt, al);
        if(len != -1)
        {
            m_ss << std::string(buf, len);
            free(buf);
        }
    }

    class MessageFormatItem : public LogFormatter::FormatItem   //输出日志内容
    {
    public:
        MessageFormatItem(const std::string & str=""){}
        void format(std::ostream & os,std::shared_ptr<Logger> logger,LogLevel::Level level,LogEvent::ptr event) override  //返回日志事件
        {
            os<<event->getContent();
        }
    };
    class LevelFormatItem : public LogFormatter::FormatItem  //输出日志级别
    {
    public:
        LevelFormatItem(const std::string & str=""){}
        void format(std::ostream & os,std::shared_ptr<Logger> logger,LogLevel::Level level,LogEvent::ptr event) override  //返回日志事件
        {
            os<<LogLevel::ToString(level);
        }
    };
    class ElapseFormatItem : public LogFormatter::FormatItem   //输出程序启动开始到现在的毫秒数
    {
    public:
        ElapseFormatItem(const std::string & str=""){}
        void format(std::ostream & os,std::shared_ptr<Logger> logger,LogLevel::Level level,LogEvent::ptr event) override  //返回日志事件
        {
            os<<event->getElapse();
        }
    };
    class NameFormatItem : public LogFormatter::FormatItem    //输出日志名称
    {
    public:
        NameFormatItem(const std::string & str=""){}
        void format(std::ostream & os,std::shared_ptr<Logger> logger,LogLevel::Level level,LogEvent::ptr event) override  //返回日志事件
        {
            os<<event->getLogger()->getName();
        }
    };
    class ThreadIdFormatItem : public LogFormatter::FormatItem    //输出线程ID
    {
    public:
        ThreadIdFormatItem(const std::string & str=""){}
        void format(std::ostream & os,std::shared_ptr<Logger> logger,LogLevel::Level level,LogEvent::ptr event) override  //返回日志事件
        {
            os<<event->getThreadId();
        }
    };
    class ThreadNameFormatItem : public LogFormatter::FormatItem    //输出线程名称
    {
    public:
        ThreadNameFormatItem(const std::string& str = "") {}
        void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override //返回日志事件
        {
            os << event->getThreadName();
        }
    };
    class FiberIdFormatItem : public LogFormatter::FormatItem    //输出协程ID
    {
    public:
        FiberIdFormatItem(const std::string & str=""){}
        void format(std::ostream & os,std::shared_ptr<Logger> logger,LogLevel::Level level,LogEvent::ptr event) override  //返回日志事件
        {
            os<<event->getFiberId();
        }
    };
    class DateTimeFormatItem : public LogFormatter::FormatItem    //输出日志时间戳
    {
    public:
        DateTimeFormatItem(const std::string & format="%Y-%m-%d %H:%M:%S"):m_format(format)  //初始化
        {
            if(m_format.empty())        //变量已存在、非空字符串或者非零，则返回 false 值
            {
                m_format="%Y-%m-%d %H:%M:%S";
            }
        }
        void format(std::ostream & os,std::shared_ptr<Logger> logger,LogLevel::Level level,LogEvent::ptr event) override  //返回日志事件
        {
            struct tm tm;   //struct tm布夫
            time_t time=event->getTime();
            localtime_r(&time,&tm); //将时间戳（time_t）转换为本地时间
            char buf[64];
            strftime(buf,sizeof(buf),m_format.c_str(),&tm);//根据m_format 中定义的格式化规则，格式化结构tm表示的时间，并把它存储在buff中。
            os<<buf;
        }
    private:
        std::string m_format;   //时间戳格式
    };
    class FilenameFormatItem : public LogFormatter::FormatItem    //输出文件名
    {
    public:
        FilenameFormatItem(const std::string & str=""){}
        void format(std::ostream & os,std::shared_ptr<Logger> logger,LogLevel::Level level,LogEvent::ptr event) override  //返回日志事件
        {
            os<<event->getFile();
        }
    };
    class LineFormatItem : public LogFormatter::FormatItem    //输出行号
    {
    public:
        LineFormatItem(const std::string & str=""){}
        void format(std::ostream & os,std::shared_ptr<Logger> logger,LogLevel::Level level,LogEvent::ptr event) override  //返回日志事件
        {
            os<<event->getLine();
        }
    };
    class NewLineFormatItem : public LogFormatter::FormatItem    //输出换行\n
    {
    public:
        NewLineFormatItem(const std::string & str=""){}
        void format(std::ostream & os,std::shared_ptr<Logger> logger,LogLevel::Level level,LogEvent::ptr event) override  //返回日志事件
        {
            os<<std::endl;
        }
    };
    class StringFormatItem : public LogFormatter::FormatItem    //输出线程名称**
    {
    public:
        StringFormatItem(const std::string &str):m_string(str){}

        void format(std::ostream & os,std::shared_ptr<Logger> logger,LogLevel::Level level,LogEvent::ptr event) override  //返回日志事件
        {
            os<<m_string;
        }
    private:
        std::string m_string;
    };
    class TabFormatItem : public LogFormatter::FormatItem    //输出制表符\t
    {
    public:
        TabFormatItem(const std::string &str =""){};

        void format(std::ostream & os,std::shared_ptr<Logger> logger,LogLevel::Level level,LogEvent::ptr event) override  //返回日志事件
        {
            os<<"\t";
        }
    private:
        std::string m_string;
    };

    LogEvent::LogEvent(std::shared_ptr<Logger>logger,LogLevel::Level level
        ,const char * file,int32_t line,uint32_t elapse
        ,uint32_t thread_id,uint32_t fiber_id,uint64_t time
        ,const std::string& thread_name)  //日志事件初始化
    :m_file(file)
    ,m_line(line)
    ,m_elapse(elapse)
    ,m_threadId(thread_id)
    ,m_fiberID(fiber_id)
    ,m_time(time)
    ,m_logger(logger)
    ,m_level(level)
    ,m_threadName(thread_name)
    {}

    LogEvent::~LogEvent(){}

    LogEventWrap::LogEventWrap(LogEvent::ptr e):m_event(e)  //构造函数
    {}
    LogEventWrap::~LogEventWrap()   //析构函数
    {
        m_event->getLogger()->log(m_event->getLevel(),m_event);
    }
    std::stringstream & LogEventWrap::getSS()//获取日志内容流
    {
        return m_event->getSS();
    }

    Logger::Logger(const std::string & name):m_name(name),m_level(LogLevel::DEBUG)  //默认最低级
    {
        m_formatter.reset(new LogFormatter("%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"));  //日志格式器的数据
    }

    void Logger::log(LogLevel::Level level,LogEvent::ptr event)  //写日志   将日志数据在日志输出目标以格式输出
    {
        if(level>=m_level)
        {
            auto self=shared_from_this();  //返回一个当前类的std::share_ptr
            MutexType::Lock lock(m_mutex);  //加互斥量
            if(!m_appender.empty()) //不为空
            {
                for(auto & i:m_appender)   //遍历容器元素
                {
                    i->log(self,level,event);
                }
            }
            else if(m_root) //为空找m_root
            {
                m_root->log(level,event);
            }
        }
    }

    void Logger::debug(LogEvent::ptr event) //写debug级别日志
    {
        log(LogLevel::DEBUG,event);
    }
    void Logger::info(LogEvent::ptr event)  //写info级别日志
    {
        log(LogLevel::INFO,event);
    }
    void Logger::warn(LogEvent::ptr event)  //写warn级别日志
    {
        log(LogLevel::WARN,event);
    }
    void Logger::error(LogEvent::ptr event) //写error级别日志
    {   
        log(LogLevel::ERROR,event);
    }
    void Logger::fatal(LogEvent::ptr event) //写fatal级别日志
    {
        log(LogLevel::FATAL,event);
    }

    void Logger::addAppender(LogAppender::ptr appender)   //Appender（日志输出目标）集合的插入
    {
        MutexType::Lock lock(m_mutex);  //加互斥量
        if(!appender->getFormatter())
        {
            MutexType::Lock ll(appender->m_mutex); //加互斥量
            appender->m_formatter=m_formatter;
        }
        m_appender.push_back(appender);
    }
    void Logger::delAppender(LogAppender::ptr appender)   //Appender（日志输出目标）集合的删除
    {
        MutexType::Lock lock(m_mutex);  //加互斥量
        for(auto it=m_appender.begin();it!=m_appender.end();++it)
        {
            if(*it==appender)
            {
                m_appender.erase(it);   //删除list元素
                break;
            }
        }
    }
    void Logger::clearAppenders()                         //清空日志目标
    {
        MutexType::Lock lock(m_mutex);  //加互斥量
        m_appender.clear();
    }

    void Logger::setFormatter(LogFormatter::ptr val)           //设置日志格式器
    {
        MutexType::Lock lock(m_mutex);  //加互斥量
        m_formatter=val;
        for(auto& i : m_appender)
        {
            MutexType::Lock ll(i->m_mutex); //加互斥量
            if(!i->m_hasFormatter) 
            {
                i->m_formatter=m_formatter;
            }
        }
    }
    void Logger::setFormatter(const std::string& val)          //设置日志格式模板
    {
        sylar::LogFormatter::ptr new_val(new sylar::LogFormatter(val));
        if(new_val->isError())  //判断是否有错误
        {
            std::cout<<"Logger setFormatter name="<<m_name
                     <<"value="<<val<<"invalid formatter"
                     <<std::endl;
            return;
        }
        setFormatter(new_val);    //设置日志格式器指针
    }
    LogFormatter::ptr Logger::getFormatter()                   //获取日志格式器
    {
        MutexType::Lock lock(m_mutex);  //加互斥量
        return m_formatter;
    }

    std::string Logger::toYamlString()//将所有的日志器配置转成YAML String
    {
        MutexType::Lock lock(m_mutex);  //加互斥量
        YAML::Node node;
        node["name"]=m_name;
        if(m_level!=LogLevel::UNKNOW)   //不是默认等级
        {
            node["level"]=LogLevel::ToString(m_level);
        }
        if(m_formatter)
        {
            node["formatter"]=m_formatter->getPattern();
        }

        for(auto& i : m_appender)
        {
            node["appenders"].push_back(YAML::Load(i->toYamlString())); //Load函数将 参数string 转换成yaml::node类型
        }
        std::stringstream ss;
        ss<<node;   //stringstream流 不只是放字符串
        return ss.str();
    }

    void LogAppender::setFormatter(LogFormatter::ptr val) //更改日志格式器   设置输出的格式
    {
        MutexType::Lock lock(m_mutex);  //加互斥量
        m_formatter=val;
        if(m_formatter)
        {
            m_hasFormatter=true;
        }else
        {
            m_hasFormatter=false;
        }
    }
    LogFormatter::ptr LogAppender::getFormatter()   //获取日志格式器   返回输出的格式
    {
        MutexType::Lock lock(m_mutex);  //加互斥量
        return m_formatter;
    }

    //打印到控制台输出格式
    void StdoutLogAppender::log(std::shared_ptr<Logger> logger,LogLevel::Level level,LogEvent::ptr event)   //覆写（override）虚函数,不用virtual
    {
        if(level>=LogAppender::m_level)   //日志等级大于原本的
        {
            MutexType::Lock lock(m_mutex);  //加互斥量
            std::cout<<LogAppender::m_formatter->format(logger,level,event);   //输出等级
        }
    }
    std::string StdoutLogAppender::toYamlString()   //将日志输出目标的配置转成YAML String
    {
        MutexType::Lock lock(m_mutex);  //加互斥量
        YAML::Node node;
        node["type"]="StdoutLogAppender";
        if(m_level!=LogLevel::UNKNOW)   //不是默认等级
        {
            node["level"]=LogLevel::ToString(m_level);
        }
        if(m_hasFormatter && m_formatter)   //m_formatter为空
        {
            node["formatter"]=m_formatter->getPattern();
        }
        std::stringstream ss;
        ss<<node;
        return ss.str();
    }

    bool FileLogAppender::reopen()   //清除——重新打开日志文件,文件打开成功返回true
    {
        MutexType::Lock lock(m_mutex);  //加互斥量
        if(m_filestream)
        {
            m_filestream.close();
        }
        m_filestream.open(m_filename,std::ios::app);    //std::ios::app文件追加
        return !!m_filestream;   //!!非0值是true,0值的是false
    }
    FileLogAppender::FileLogAppender(const std::string & filename):m_filename(filename)//初始化
    {
        reopen();   //清除——重新打开日志文件
    }
    //打印到文件输出格式
    void FileLogAppender::log(std::shared_ptr<Logger> logger,LogLevel::Level level,LogEvent::ptr event)   //覆写（override）虚函数,不用virtual
    {
        if(level >= m_level) 
        {
            uint64_t now = event->getTime();
            if(now >= (m_lastTime + 3))
            {
                reopen();   //清除
                m_lastTime = now;
            }
            MutexType::Lock lock(m_mutex);  //加互斥量
            if(!m_formatter->format(m_filestream, logger, level, event))
            {
                std::cout << "error:The log directory does not exist" << std::endl;
            }
        }
    }
    std::string FileLogAppender::toYamlString()   //将日志输出目标的配置转成YAML String
    {
        MutexType::Lock lock(m_mutex);  //加互斥量
        YAML::Node node;
        node["type"]="FileLogAppender";
        node["file"]=m_filename;
        if(m_level!=LogLevel::UNKNOW)   //不是默认等级
        {
            node["level"]=LogLevel::ToString(m_level);
        }
        if(m_hasFormatter && m_formatter)
        {
            node["formatter"]=m_formatter->getPattern();
        }
        std::stringstream ss;
        ss<<node;
        return ss.str();
    }

    LogFormatter::LogFormatter(const std::string & pattern):m_pattern(pattern)  //构造函数  初始化 
    {
        init();
    }
    std::string LogFormatter::format(std::shared_ptr<Logger> logger,LogLevel::Level level,LogEvent::ptr event)  //返回格式化日志文本
    {
        std::stringstream ss;  //stringstream 流输入输出
        for(auto & i : m_items)
        {
            i->format(ss,logger,level,event);   //并且返回字符串类型
        }
        return ss.str();
    }
    std::ostream& LogFormatter::format(std::ostream& ofs, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event)    //返回格式化日志文件（文件）
    {
        for(auto& i : m_items) 
        {
            i->format(ofs, logger, level, event);
        }
        return ofs;
    }

    void LogFormatter::init()  //初始化,解析日志格式模板     Appender的解析
    {
    //"%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"
    //{%Y-%m-%d %H:%M:%S} 时间格式
        //str,format,type
        /*
        第一string是%x
        第二string是{xxx}
        第三int是区分是%x%x --0
                      %x{xxx} --1
        */
        std::vector<std::tuple<std::string,std::string,int>> vec;   //tuple是一个固定大小的不同类型值的集合
        std::string nstr;   //存%x%x
        for(size_t i=0;i<m_pattern.size();++i)
        {
            if(m_pattern[i]!='%')
            {
                nstr.append(1,m_pattern[i]);  //%前面数据存储   append追加
                continue;
            }

            if((i+1)<m_pattern.size())  //①标记刚解析完%X{xxx}---标记%x%x  ②标记[xx]    ③标记 :%x
            {
                if(m_pattern[i+1]=='%')
                {
                    nstr.append(1,'%');  //标记%x%x的情况   append追加
                    continue;
                }
            }

            size_t n=i+1;
            int fmt_status=0;  //部分格式标记   0-%x
            size_t fmt_bengin=0; //标记位置

            std::string str;  //%x{}  //存x
            std::string fmt;  //{xxx}   //存xxx
            //"%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"
            while (n<m_pattern.size())  //解析%x{xxx}
            {
                if(!fmt_status && (!isalpha(m_pattern[n]) && m_pattern[n] != '{'
                    && m_pattern[n] != '}')) 
                    {
                        str = m_pattern.substr(i + 1, n - i - 1);
                        break;
                    }
                if(fmt_status==0)   //解析%x
                {
                    if(m_pattern[n]=='{')
                    {
                        str=m_pattern.substr(i+1,n-i-1);  //存%x{}中的x    substr复制子字符串,参数：开始位置，子字符串的数目
                        //std::cout<<"*"<<str<<std::endl;
                        fmt_status=1; //标记已找到{
                        fmt_bengin=n;//标记{ 的位置
                        ++n;
                        continue;
                    }
                }
                else if(fmt_status==1)  //解析{xxx}  取{xxx}中的元素xxx
                {
                    if(m_pattern[n]=='}')
                    {
                        fmt=m_pattern.substr(fmt_bengin+1,n-fmt_bengin-1);//取{xxx}中的元素     substr复制子字符串
                        //std::cout << "#" << fmt << std::endl;
                        fmt_status=0;
                        ++n;
                        break;
                    }
                }
                ++n;
                if(n == m_pattern.size()) 
                {
                    if(str.empty()) 
                    {
                        str = m_pattern.substr(i + 1);
                    }
                }
            }
            //"%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"
            if(fmt_status==0)  //并未解析到 %x{xxx}
            {
                if(!nstr.empty()) //测试nstr变量是否已经配置,已 非0
                {
                    vec.push_back(std::make_tuple(nstr,std::string(),0));
                    nstr.clear();
                }
                //str=m_pattern.substr(i+1,n-i-1);//substr复制子字符串,参数：开始位置，子字符串的数目
                vec.push_back(std::make_tuple(str,fmt,1));
                i=n-1;
            }
            else if(fmt_status==1)  //格式错误
            {
                std::cout<<"pattern parse error:"<<m_pattern<<"-"<<m_pattern.substr(i)<<std::endl;
                m_error = true;
                vec.push_back(std::make_tuple("<pattern_error>",fmt,0));
            }

        }
        if(!nstr.empty())//测试nstr变量是否已经配置,已 非0
        {
            vec.push_back(std::make_tuple(nstr,"",0));
        }
        static std::map<std::string,std::function<FormatItem::ptr(const std::string & str)>> s_format_items=
        {//function 是通用多态函数封装器
    #define XX(str,C) \
            {#str,[](const std::string &fmt) {return FormatItem::ptr(new C(fmt));}} //{#str,[](const std::string &fmt)     通过%str访问 return FormatItem::ptr(new C(fmt));
            
            XX(m,MessageFormatItem),    //m:消息
            XX(p,LevelFormatItem),      //p:日志级别
            XX(r,ElapseFormatItem),     //r:累计毫秒数
            XX(c,NameFormatItem),       //c:日志名称
            XX(t,ThreadIdFormatItem),   //t:线程id
            XX(n,NewLineFormatItem),    //n:换行
            XX(d,DateTimeFormatItem),   //d:时间
            XX(f,FilenameFormatItem),   //f:文件名
            XX(l,LineFormatItem),       //l:行号
            XX(T,TabFormatItem),        //T:Tab
            XX(F,FiberIdFormatItem),    //F:协程id
            XX(N, ThreadNameFormatItem),//N:线程名称
    #undef XX
        };
        for(auto & i:vec)
        {
            if(std::get<2>(i)==0) //get是在vec数组中下标为i的集合中的第2+1个元素
            {
                m_items.push_back(FormatItem::ptr(new StringFormatItem(std::get<0>(i))));
            }
            else
            {
                auto it=s_format_items.find(std::get<0>(i));
                if(it==s_format_items.end())
                {
                    m_items.push_back(FormatItem::ptr(new StringFormatItem("error_format %"+std::get<0>(i)+">>")));
                    m_error=true;
                }
                else
                {
                    m_items.push_back(it->second(std::get<1>(i)));
                }
            }
        }
        //std::cout<<m_items.size()<<std::endl;
        //%m--消息体
        //%p--leve
        //%r--启动后的时间
        //%c--日志名称
        //%t--线程ID
        //%n--回车换行
        //%d--时间
        //%f--文件名
        //%l--行号
    }

    LoggerManager::LoggerManager()  //构造函数
    {
        m_root.reset(new Logger);   //reset为智能指针（m_root）要指向的新对象
        m_root->addAppender(LogAppender::ptr(new StdoutLogAppender));   //主日志器指向控制台日志输出
        
        m_loggers[m_root->m_name]=m_root;   //插入主m_root日志器容器

        init();
    }
    
    Logger::ptr LoggerManager::getLogger(const std::string &name)   //获取日志器(没有则创建日志器)    name 日志器名称 
    {
        MutexType::Lock lock(m_mutex);  //加互斥量
        auto it=m_loggers.find(name); 
        if(it!=m_loggers.end()) //日志器存在
        {
            return it->second;
        }
        Logger::ptr logger(new Logger(name));   //创建日志器
        logger->m_root=m_root;  
        m_loggers[name]=logger;
        return logger;
    }
    void LoggerManager::init() //初始化
    {}

    struct LogAppenderDefine    //日志器输出地的属性（新旧）————和配置系统整合
    {
        int type=0; //判断输出地  1 File , 2 Stdout
        LogLevel::Level level=LogLevel::UNKNOW;
        std::string formatter;
        std::string file;

        bool operator==(const LogAppenderDefine& oth) const
        {
            return type==oth.type
                && level==oth.level
                && formatter==oth.formatter
                && file==oth.file;
        }
    };

    struct LogDefine    //日志器的属性（新旧）————和配置系统整合
    {
        std::string name;
        LogLevel::Level level=LogLevel::UNKNOW;
        std::string formatter;
        std::vector<LogAppenderDefine> appenders;

        bool operator==(const LogDefine& oth) const
        {
            return name==oth.name
                && level==oth.level
                && formatter==oth.formatter
                && appenders==appenders;
        }

        bool operator<(const LogDefine& oth) const
        {
            return name<oth.name;
        }
    };

    /**
     *  类型转换模板类片特化(YAML String 转换成 std::set<LogDefine>)
     */
    template<>
    class LexicalCast<std::string,std::set<LogDefine>>   //模块重载
    {
    public:
        std::set<LogDefine> operator()(const std::string& v)
        {
            YAML::Node node=YAML::Load(v);  //将string类型转化成yaml数组
            std::set<LogDefine> vec; 
            for(size_t i=0;i<node.size();++i)   //将yaml数组转换成std::set<LogDefine>类型
            {
                auto n=node[i];
                if(!n["name"].IsDefined())  //IsDefined判断在不在isdefined,在1 不在0   --name是唯一标识
                {
                    std::cout<<"log config error: name is null, "<<n<<std::endl;
                    continue;
                }

                LogDefine ld;
                ld.name=n["name"].as<std::string>();    //as把node里的类型变成 对应string类型

                ld.level=LogLevel::FromString( n ["level"].IsDefined() ? n["level"].as<std::string>() : "");

                if(n["formatter"].IsDefined())
                {
                    ld.formatter=n["formatter"].as<std::string>();
                }

                if(n["appenders"].IsDefined())  //处理appenders
                {
                    for(size_t x=0;x<n["appenders"].size();++x) //set是二叉树
                    {
                        auto a=n["appenders"][x];
                        if(!a["type"].IsDefined())
                        {
                            std::cout<<"log config error: appender type is null, "<<a<<std::endl;
                            continue;
                        }
                        std::string type=a["type"].as<std::string>();
                        LogAppenderDefine lad;
                        if(type=="FileLogAppender") //文件输出地-转换
                        {
                            lad.type=1;
                            if(!a["file"].IsDefined())
                            {
                                std::cout<<"log config error: fileappender file is bull, "<<a<<std::endl;
                                continue;
                            }
                            lad.file=a["file"].as<std::string>();
                            if(a["formatter"].IsDefined())
                            {
                                lad.formatter=a["formatter"].as<std::string>();
                            }
                        }else if(type=="StdoutLogAppender") //终端输出地-转换
                        {
                            lad.type=2;
                        }else
                        {
                            std::cout<<"log config error: appender type is invalid, "<<a<<std::endl;
                            continue;
                        }
                        ld.appenders.push_back(lad);//插入日志输出地
                    }
                }
                vec.insert(ld);
            }
            return vec;
        }
    };

    /**
     *  类型转换模板类片特化(std::set<LogDefine> 转换成 YAML String)
     */
    template<>
    class LexicalCast<std::set<LogDefine>,std::string>   //模块重载
    {
    public:
        std::string operator()(const std::set<LogDefine>& v)
        {
            YAML::Node node;  
            for(auto& i:v)
            {
                YAML::Node n;
                n["name"]=i.name;

                if(i.level!=LogLevel::UNKNOW)   //不是默认值
                {
                    n["level"]=LogLevel::ToString(i.level);
                }

                if(!i.formatter.empty())
                {
                    n["formatter"]=i.formatter;
                }

                for(auto& a: i.appenders)   //appenders里面有level和formatter
                {
                    YAML::Node na;
                    if(a.type==1)
                    {
                        na["type"]="FileLogAppender";
                        na["file"]=a.file;
                    }
                    else if(a.type==2)
                    {
                        na["type"]="StdoutLogAppender";
                    }
                    if(a.level!=LogLevel::UNKNOW)
                    {
                        na["level"]=LogLevel::ToString(a.level);
                    }
                    if(!a.formatter.empty())
                    {
                        na["formatter"]=a.formatter;
                    }
                    n["appenders"].push_back(na);
                }
                node.push_back(n);  //node相当与map函数
            }
            std::stringstream ss;
            ss<<node;
            return ss.str();
        }
    };

    //创建set<LogDefine>类型配置对象
    sylar::ConfigVar<std::set<LogDefine> >::ptr g_log_defines=
        sylar::Config::Lookup("logs",std::set<LogDefine>(),"logs config");

    struct LogIniter    //变更事件(修改日志器的配置)
    {
        LogIniter()
        {
            g_log_defines->addListener([](const std::set<LogDefine>& old_value,const std::set<LogDefine>& new_value)
            {
                SYLAR_LOG_INFO(SYLAR_LOG_ROOT())<<"on_logger_conf_changed";
                /*新增————新的有，旧的没有
                修改————新旧都有，但是不一样
                删除————旧的有，新的没有*/
                for(auto& i:new_value) 
                {
                    auto it=old_value.find(i);    //看旧的有没有
                    sylar::Logger::ptr logger;
                    if(it==old_value.end())
                    {//新增的logger
                        logger=SYLAR_LOG_NAME(i.name);   //搜索要修改的logger
                    }else
                    {
                        if(!(i==*it))   //LogDefine仿函数
                        {//修改的logger
                            logger=SYLAR_LOG_NAME(i.name);   //搜索要修改的logger
                        }else
                        {
                            continue;
                        }
                    }
                    logger->setLevel(i.level);
                    if(!i.formatter.empty()) //空的话，是默认
                    {
                        logger->setFormatter(i.formatter);
                    }

                    logger->clearAppenders();
                    for(auto& a:i.appenders)
                    {
                        sylar::LogAppender::ptr ap;
                        if(a.type==1)   //输出到文件
                        {
                            ap.reset(new FileLogAppender(a.file));
                        }else if(a.type==2) //输出到控制台
                        {
                            ap.reset(new StdoutLogAppender);
                        }
                        ap->setLevel(a.level);
                        if(!a.formatter.empty())
                        {
                            LogFormatter::ptr fmt(new LogFormatter(a.formatter));
                            if(!fmt->isError())
                            {
                                ap->setFormatter(fmt);
                            }else
                            {
                                std::cout<<"log.name="<<i.name<<"appender type="<<a.type
                                         <<"formatter="<<a.formatter<<" is invalid"<<std::endl;
                            }
                        }
                        logger->addAppender(ap);
                    }
                }
                for(auto& i:old_value)
                {
                    auto it=new_value.find(i);
                    if(it==new_value.end())
                    {//删除的logger————不是真删除，相当于删除
                        auto logger=SYLAR_LOG_NAME(i.name); 
                        logger->setLevel((LogLevel::Level)100); //设置很高的等级
                        logger->clearAppenders();
                    }
                }
            });
        }
    };

    struct LogIniter __log_init;

    std::string LoggerManager::toYamlString() //将所有的日志器配置转成YAML String
    {
        MutexType::Lock lock(m_mutex);  //加互斥量
        YAML::Node node;
        for(auto& i : m_loggers)
        {
            node.push_back(YAML::Load(i.second->toYamlString()));
        }
        std::stringstream ss;
        ss<<node;
        return ss.str();
    }
}