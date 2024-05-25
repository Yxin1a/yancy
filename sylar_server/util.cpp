#include"util.h"
#include<execinfo.h>
#include<malloc.h>
#include<sys/time.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<dirent.h>
#include<string.h>
#include<stdlib.h>
#include<ifaddrs.h>

#include"log.h"
#include"fiber.h"

namespace sylar
{
    sylar::Logger::ptr g_logger=SYLAR_LOG_NAME("system");

    pid_t GetThreadId()     //返回当前线程ID
    {
        return syscall(SYS_gettid);
    }
    
    u_int32_t GetFiberId()  //返回当前协程ID
    {
        return sylar::Fiber::GetFiberId();
    }

    void Backtrace(std::vector<std::string>& bt,int size,int skip)  //获取当前的调用栈
    {
        void** array=(void**)malloc((sizeof(void*)* size));     //指针数组————void指针的指针类型（二级指针） ,地址指向空间的是一个地址
        size_t s=::backtrace(array,size);   //backtrace()获取当前线程的调用堆栈，获取的信息将会被存放在array中，array是一个指针数组，
                                            //参数size用来指定array中可以保存多少个void*元素。
                                            //返回值是实际返回的void*元素个数。

        char** strings=backtrace_symbols(array,s);  //backtrace_symbols()将backtrace函数获取的信息转化为一个字符串数组，
                                                    //参数array是backtrace()获取的堆栈指针
                                                    //参数s是backtrace()返回值。
        if(strings==NULL)
        {
            SYLAR_LOG_ERROR(g_logger)<<"backtrace_symbols error";
            return;
        }

        for(size_t i=skip;i<s;++i)
        {
            bt.push_back(strings[i]);
        }
        free(strings);
        free(array);
    }

    std::string BacktraceToString(int size,int skip,const std::string& prefix)  //获取当前栈信息的字符串
    {
        std::vector<std::string> bt;    //用来保存调用栈的
        Backtrace(bt,size,skip);
        std::stringstream ss;
        for(size_t i=0;i<bt.size();++i)
        {
            ss<<prefix<<bt[i]<<std::endl;
        }
        return ss.str();
    }

    uint64_t GetCurrentMS() //获取当前时间的毫秒级
    {
        struct timeval tv;
        gettimeofday(&tv,NULL);     //gettimeofday需要获得当前精确时间
        return tv.tv_sec*1000ul+tv.tv_usec/1000;  //单位：tv_sec秒数  tv_usec微秒数
    }

    uint64_t GetCurrentUS() //获取当前时间的微秒级
    {
        struct timeval tv;
        gettimeofday(&tv, NULL);    //gettimeofday需要获得当前精确时间
        return tv.tv_sec*1000*1000ul+tv.tv_usec; //单位：tv_sec秒数  tv_usec微秒数
    }

    std::string Time2Str(time_t ts, const std::string& format)  //将时间戳以format格式返回字符串
    {
        struct tm tm;
        localtime_r(&ts, &tm);
        char buf[64];
        strftime(buf, sizeof(buf), format.c_str(), &tm);
        return buf;
    }

    time_t Str2Time(const std::string& timeStr,const std::string& format) //将以format格式的时间字符串返回对应时间戳
    {
        struct tm tm;
        strptime(timeStr.c_str(),format.c_str(),&tm);
        time_t timeStamp = mktime(&tm);
        return timeStamp;
    }

    void FSUtil::ListAllFile(std::vector<std::string>& files    //获取path目录下符合subifx后缀的文件，存储到files中
                            ,const std::string& path
                            ,const std::string& subfix)
    {
        if(access(path.c_str(), 0) != 0)    //access()判断目录是否存在
        {
            return;
        }
        DIR* dir = opendir(path.c_str());   //打开目录
        if(dir == nullptr)
        {
            return;
        }
        struct dirent* dp = nullptr;
        while((dp = readdir(dir)) != nullptr)   //readdir()返回在参数dir⽬录流(dir目录尾部)下的目录、文件
                                                //  **再次调用就指向同一级别下一个的目录或者文件
        {
            if(dp->d_type == DT_DIR)    //是文件夹      d_type指向的文件类型
            {
                if(!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, ".."))   //d_name下一个目录名(子目录)    .当前路径   .. 上一个了
                {
                    continue;   //指向同一级别下一个的目录或者文件
                }
                ListAllFile(files, path + "/" + dp->d_name, subfix);    //递归访问path/dp->d_name(子目录)
            }
            else if(dp->d_type == DT_REG)   //是文件
            {
                std::string filename(dp->d_name);   //d_name文件名
                if(subfix.empty())  //没要求的后缀
                {
                    files.push_back(path + "/" + filename);
                }
                else
                {
                    if(filename.size() < subfix.size()) //文件不符合subfix后缀
                    {
                        continue;   //指向同一级别下一个的目录或者文件
                    }
                    if(filename.substr(filename.length() - subfix.size()) == subfix)    //看文件是否和subfix一个后缀
                    {
                        files.push_back(path + "/" + filename);
                    }
                }
            }
        }
        closedir(dir);  //释放
    }

    static int __lstat(const char* file, struct stat* st = nullptr) //用来获取linux操作系统下文件的属性(是否存在)   0 存在
    {
        struct stat lst;
        int ret = lstat(file, &lst);    //用来获取linux操作系统下文件的属性
        if(st)
        {
            *st = lst;
        }
        return ret;
    }

    static int __mkdir(const char* dirname) //创建目录，定义权限
    {
        if(access(dirname, F_OK) == 0)  //只判断是否有读权限
        {
            return 0;
        }// 空格为结尾
        return mkdir(dirname, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);   //创建目录(只能在已存在的目录下建立一级子目录)，定义权限,成功返回0
    }

    bool FSUtil::Mkdir(const std::string& dirname)  //创建目录
    {   //  /home/yangxin目录
        if(__lstat(dirname.c_str()) == 0)   //存在
        {
            return true;
        }
        char* path = strdup(dirname.c_str());   //copy
        char* ptr = strchr(path + 1, '/');  //查找给定字符的第一个匹配之处————home/
        do
        {
            for(; ptr; *ptr = '/', ptr = strchr(ptr + 1, '/'))  //子目录创建的次数
            {   
                *ptr = '\0';
                //  home yangxin
                if(__mkdir(path) != 0)  //已存在，不能再创建了
                {
                    break;
                }
            }
            if(ptr != nullptr)  //  /home/yangxin/  错误
            {
                break;
            }
            else if(__mkdir(path) != 0) //创建yangxin目录
            {
                break;
            }
            free(path);
            return true;
        } while(0);
        free(path);
        return false;
    }

    bool FSUtil::IsRunningPidfile(const std::string& pidfile)   //判断进程pid文件是否存在且正在运行
    {
        if(__lstat(pidfile.c_str()) != 0)   //用来获取linux操作系统下文件的属性
        {
            return false;
        }
        std::ifstream ifs(pidfile);
        std::string line;
        if(!ifs || !std::getline(ifs, line))
        {
            return false;
        }
        if(line.empty())
        {
            return false;
        }
        pid_t pid = atoi(line.c_str());
        if(pid <= 1)
        {
            return false;
        }
        if(kill(pid, 0) != 0)
        {
            return false;
        }
        return true;
    }

    int8_t  TypeUtil::ToChar(const std::string& str) 
    {
        if(str.empty())
        {
            return 0;
        }
        return *str.begin();
    }

    int64_t TypeUtil::Atoi(const std::string& str) 
    {
        if(str.empty()) 
        {
            return 0;
        }
        return strtoull(str.c_str(), nullptr, 10);
    }

    double  TypeUtil::Atof(const std::string& str) 
    {
        if(str.empty()) 
        {
            return 0;
        }
        return atof(str.c_str());
    }

    int8_t  TypeUtil::ToChar(const char* str) 
    {
        if(str == nullptr) 
        {
            return 0;
        }
        return str[0];
    }

    int64_t TypeUtil::Atoi(const char* str) 
    {
        if(str == nullptr) 
        {
            return 0;
        }
        return strtoull(str, nullptr, 10);
    }

    double  TypeUtil::Atof(const char* str) 
    {
        if(str == nullptr) 
        {
            return 0;
        }
        return atof(str);
    }

    std::string GetHostName() 
    {
        std::shared_ptr<char> host(new char[512], sylar::delete_array<char>);
        memset(host.get(), 0, 512);
        gethostname(host.get(), 511);   //获取主机名称
        return host.get();
    }

    in_addr_t GetIPv4Inet() 
    {
        struct ifaddrs* ifas = nullptr;
        struct ifaddrs* ifa = nullptr;

        in_addr_t localhost = inet_addr("127.0.0.1");
        if(getifaddrs(&ifas)) 
        {
            SYLAR_LOG_ERROR(g_logger) << "getifaddrs errno=" << errno
                << " errstr=" << strerror(errno);
            return localhost;
        }

        in_addr_t ipv4 = localhost;

        for(ifa = ifas; ifa && ifa->ifa_addr; ifa = ifa->ifa_next) 
        {
            if(ifa->ifa_addr->sa_family != AF_INET) 
            {
                continue;
            }
            if(!strncasecmp(ifa->ifa_name, "lo", 2)) 
            {
                continue;
            }
            ipv4 = ((struct sockaddr_in*)ifa->ifa_addr)->sin_addr.s_addr;
            if(ipv4 == localhost) 
            {
                continue;
            }
        }
        if(ifas != nullptr) 
        {
            freeifaddrs(ifas);
        }
        return ipv4;
    }

    std::string _GetIPv4() 
    {
        std::shared_ptr<char> ipv4(new char[INET_ADDRSTRLEN], sylar::delete_array<char>);
        memset(ipv4.get(), 0, INET_ADDRSTRLEN);
        auto ia = GetIPv4Inet();
        inet_ntop(AF_INET, &ia, ipv4.get(), INET_ADDRSTRLEN);
        return ipv4.get();
    }

    std::string GetIPv4() 
    {
        static const std::string ip = _GetIPv4();
        return ip;
    }

}