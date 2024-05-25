//hook某些特殊时刻，系统内部预先设置好的函数，当系统周期到达指定时刻 会自动执行该'钩子'
//(就是在socket、io、fd相关函数(有读有写)中有超时时间，就加个定时器(等待时间)，超出等待时间后执行对应事件函数，并退出)
//(在socket、io、fd相关函数中与IOManager的事件绑定，使得调用的时候触发)
//句柄socket要在FdCtx容器存在
//socket相关(socket、connect客户连接服务、accept服务接受客户、close)
//io相关(read、readv、recv、recvfrom、recvmsg、
//      write、writev、send、sendto、sendmsg)
//fd相关(fcntl、ioctl、getsockopt获取一个套接字的选项、setsockopt设置套接字描述符的属性)
#include"hook.h"
#include"log.h"
#include"fiber.h"
#include"iomanager.h"
#include"fd_manager.h"
#include"config.h"

#include<dlfcn.h>

sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");
namespace sylar
{
    //thread_local————t_hook_enable仅可在thread_local上创建的线程上访问。用于协程本地对象的声明
    static thread_local bool t_hook_enable=false;   //当前线程是否采用hook
    
    static sylar::ConfigVar<int>::ptr g_tcp_connect_timeout=
        sylar::Config::Lookup("tcp.connect.timeout",5000,"tcp connect timeout");

    #define HOOK_FUN(XX)    \
        XX(sleep)   \
        XX(usleep)  \
        XX(nanosleep)   \
        XX(socket)  \
        XX(connect) \
        XX(accept)  \
        XX(read)    \
        XX(readv)   \
        XX(recv)    \
        XX(recvfrom)\
        XX(recvmsg) \
        XX(write)   \
        XX(writev)  \
        XX(send)    \
        XX(sendto)  \
        XX(sendmsg) \
        XX(close)   \
        XX(fcntl)   \
        XX(ioctl)   \
        XX(getsockopt)  \
        XX(setsockopt)

    void hook_init()
    {
        static bool is_inited=false;
        if(is_inited)
        {
            return;
        }
    #define XX(name) name ## _f=(name ## _fun)dlsym(RTLD_NEXT,#name);   //(定义)从系统的库取出对应的函数源码，转成我们要的方式，并赋给name
        HOOK_FUN(XX);
    #undef XX
    }

    static uint64_t s_connect_timeout = -1;

    struct _HookIniter  //要在main之前取出系统库的源码
    {
        _HookIniter()
        {
            hook_init();    //在全局变量数据初始化的时候执行函数(在main函数之前执行)
            s_connect_timeout=g_tcp_connect_timeout->getValue();

            g_tcp_connect_timeout->addListener([](const int& old_value, const int& new_value)
            {
                SYLAR_LOG_INFO(g_logger) << "tcp connect timeout changed from "
                                         << old_value << " to " << new_value;
                s_connect_timeout = new_value;
            });
        }
    };
    
    static _HookIniter s_hook_initer;   //静态全局变量在mian函数之前初始化数据

    bool is_hook_enable()  //返回当前线程是否hook
    {
        return t_hook_enable;
    }

    void set_hook_enable(bool flag)    //设置当前线程的hook状态
    {
        t_hook_enable=flag;
    }

}

    struct timer_info   //建立条件
    {
        int cancelled=0;    //存储共享指针的引用数
    };
    
    /**
     *  重新编写要hook的函数模板(socket、io、fd相关)
     *      fd              句柄
     *      fun             和socket、io、fd相关的函数
     *      hook_fun_name   要hook的函数名
     *      event           在IOmanager的事件
     *      timeout_so      在fd_manager的超时类型
     *      args            fun函数的参数
     * (有超时时间，就加个定时器(等待时间)，超出等待时间后执行对应事件函数，并退出)
     * (函数与IOManager的事件绑定，使得调用的时候触发)
    */
    template<typename OriginFun,typename... Args>
    static ssize_t do_io(int fd,OriginFun fun,const char* hook_fun_name,
                uint32_t event,int timeout_so,Args&&...args)
    {   
        //不用hook
        if(!sylar::t_hook_enable)   //当前线程不采用hook
        {   //forward(完美转发)按照参数原来的类型转发到另一个函数
            return fun(fd,std::forward<Args>(args)...); //什么都不做，把原函数返回回去
        }
        
        sylar::FdCtx::ptr ctx=sylar::FdMgr::GetInstance()->get(fd);  //获取/创建文件句柄类FdCtx
        if(!ctx)    //文件句柄不存在，不是socket
        {   //forward(完美转发)按照参数原来的类型转发到另一个函数
            return fun(fd,std::forward<Args>(args)...); //什么都不做，把原函数返回回去
        }

        if(ctx->isClose())  //文件句柄已关闭
        {
            errno=EBADF;
            return -1;
        }

        if(!ctx->isSocket() || ctx->getUserNonblock())  //不是socket 或 是用户主动设置的非阻塞(hook)
        {   //forward(完美转发)按照参数原来的类型转发到另一个函数
            return fun(fd,std::forward<Args>(args)...); //什么都不做，把原函数返回回去
        }

        //hook要做的
        uint64_t to=ctx->getTimeout(timeout_so);    //获取超时时间
        std::shared_ptr<timer_info> tinfo(new timer_info);  //建立条件

retry:  //失败继续读取标签
        ssize_t n=fun(fd,std::forward<Args>(args)...);  //先执行一遍函数
        while (n==-1 && errno==EINTR)   //失败原因：处于中断
        {
            n=fun(fd,std::forward<Args>(args)...);
        }
        if(n==-1 && errno==EAGAIN)  //失败原因：处于非阻塞(目前没有可用数据输入)，要加定时器，要做异步操作
        {
            sylar::IOManager* iom=sylar::IOManager::GetThis();  //IO
            sylar::Timer::ptr timer;
            std::weak_ptr<timer_info> winfo(tinfo); //winfo条件————借助weak_ptr类型指针可以获取shared_ptr指针的一些状态信息(比如引用计数)

            if(to!=(uint64_t)-1)    //没有超时,要加条件定时器
            {
                timer=iom->addConditionTimer(to,[winfo,fd,iom,event]()
                {   //添加条件定时器(如果对应的共享指针存在，执行定时器回调函数)
                    auto t=winfo.lock();    //返回winfo对应的存储空共享指针(timer_info)
                    if(!t | t->cancelled)   //没有触发定时器    cancelled判断是否设错误
                    {
                        return;
                    }
                    t->cancelled=ETIMEDOUT; //ETIMEDOUT超时
                    iom->cancelEvent(fd,(sylar::IOManager::Event)(event));  //超时，则执行函数
                },winfo);
            }

            int rt=iom->addEvent(fd,(sylar::IOManager::Event)(event));  //添加IOmanager事件
            if(rt)  //添加失败
            {
                SYLAR_LOG_ERROR(g_logger)<<hook_fun_name<<" addEvent("
                        <<fd<<", "<<event<<")";
                if(timer)
                {
                    timer->cancel();    //删除定时器
                }
                return -1;
            }
            else
            {
                sylar::Fiber::YieldToHold();   //将当前协程切换到后台，并将主协程切换出来,并设置为HOLD状态
                if (timer)
                {
                    timer->cancel();    //删除定时器
                }
                if(tinfo->cancelled)    //ETIMEDOUT超时
                {
                    errno=tinfo->cancelled;
                    return -1;
                }

                goto retry; //失败
            }
        }
        return n;
    }

extern "C"  //extern说明 此变量/函数是在别处定义的，要在此处引用
{
    #define XX(name) name ## _fun name ## _f=nullptr;   //(声明)函数
        HOOK_FUN(XX);
    #undef XX
    
    //sleep
    unsigned int sleep(unsigned int seconds)    //重新编写和sleep差不多的函数————秒级
    {
        if(!sylar::t_hook_enable)
        {
            return sleep_f(seconds);    //返回原来函数
        }

        sylar::Fiber::ptr fiber=sylar::Fiber::GetThis();    //返回当前协程
        sylar::IOManager* iom=sylar::IOManager::GetThis();  //返回当前iomanager
        //iom->addTimer(seconds*1000,std::bind(&sylar::IOManager::schedule,iom,fiber));   //给当前的iomananger加定时器
        iom->addTimer(seconds*1000,std::bind((void(sylar::Scheduler::*) //(void(sylar::Scheduler::*)(sylar::Fiber::ptr,int thread))定义bind模板方法
                (sylar::Fiber::ptr,int thread))& sylar::IOManager::schedule //thread是函数有默认值
                ,iom,fiber,-1));
        sylar::Fiber::YieldToHold();    //切换到主协程
        return 0;
    }

    int usleep(useconds_t usec) //重新编写和sleep差不多的函数—————微秒级
    {
        if(!sylar::t_hook_enable)
        {
            return usleep_f(usec);    //返回原函数
        }

        sylar::Fiber::ptr fiber=sylar::Fiber::GetThis();    //返回当前协程
        sylar::IOManager* iom=sylar::IOManager::GetThis();  //返回当前iomanager
        //iom->addTimer(usec/1000,std::bind(&sylar::IOManager::schedule,iom,fiber));   //给当前的iomananger加定时器
        iom->addTimer(usec/1000,std::bind((void(sylar::Scheduler::*)    //(void(sylar::Scheduler::*)(sylar::Fiber::ptr,int thread))定义bind模板方法
                (sylar::Fiber::ptr,int thread))& sylar::IOManager::schedule //thread是函数有默认值
                ,iom,fiber,-1));
        sylar::Fiber::YieldToHold();    //切换到主协程
        return 0;
    }

    int nanosleep(const struct timespec *req,struct timespec *rem)  //重新编写和sleep差不多的函数————毫秒级
    {
        if(!sylar::t_hook_enable)
        {
            return nanosleep_f(req,rem);    //返回原来函数
        }

        int timeout_ms=req->tv_sec*1000+req->tv_nsec/1000/1000; //tv_sec是秒    tv_nsec是纳秒
        sylar::Fiber::ptr fiber=sylar::Fiber::GetThis();    //返回当前协程
        sylar::IOManager* iom=sylar::IOManager::GetThis();  //返回当前iomanager
        //iom->addTimer(seconds*1000,std::bind(&sylar::IOManager::schedule,iom,fiber));   //给当前的iomananger加定时器
        iom->addTimer(timeout_ms,std::bind((void(sylar::Scheduler::*)   //(void(sylar::Scheduler::*)(sylar::Fiber::ptr,int thread))定义bind模板方法
                (sylar::Fiber::ptr,int thread))& sylar::IOManager::schedule //thread是函数有默认值
                ,iom,fiber,-1));
        sylar::Fiber::YieldToHold();    //切换到主协程
        return 0;
    }
    
    //socket
    int socket(int domain,int type,int protocol)    //重新编写socket，覆盖原本的系统函数
    {
        if(!sylar::t_hook_enable)   //当前线程不采用hook
        {
            return socket_f(domain,type,protocol);
        }
        int fd=socket_f(domain,type,protocol);
        if(fd==-1)
        {
            return fd;
        }
        sylar::FdMgr::GetInstance()->get(fd,true);  //获取文件句柄类FdCtx————自动创建，并插入容器
        return fd;
    }

    int connect_with_timeout(int fd, const struct sockaddr* addr, socklen_t addrlen, uint64_t timeout_ms)   //重新编写和connect差不多的函数(增加了超时变量)
    {
        if(!sylar::t_hook_enable)   //当前线程不采用hook
        {
            return connect_f(fd, addr, addrlen);
        }
        sylar::FdCtx::ptr ctx = sylar::FdMgr::GetInstance()->get(fd);   //获取文件句柄类FdCtx————自动创建，并插入容器
        if(!ctx || ctx->isClose())  //不存在，已关闭
        {
            errno = EBADF;
            return -1;
        }

        if(!ctx->isSocket())    //不是socket
        {
            return connect_f(fd, addr, addrlen);
        }

        if(ctx->getUserNonblock())  //用户主动设置的非阻塞 
        {
            return connect_f(fd, addr, addrlen);
        }

        //hook操作,加定时器，绑定I/O事件
        int n = connect_f(fd, addr, addrlen);
        if(n == 0)
        {
            return 0;
        }
        else if(n != -1 || errno != EINPROGRESS)    //以非阻塞的方式来进行连接
        {
            return n;
        }

        sylar::IOManager* iom = sylar::IOManager::GetThis();
        sylar::Timer::ptr timer;
        std::shared_ptr<timer_info> tinfo(new timer_info);
        std::weak_ptr<timer_info> winfo(tinfo);

        if(timeout_ms != (uint64_t)-1)  //没有超时，加条件定时器
        {
            timer = iom->addConditionTimer(timeout_ms, [winfo, fd, iom]()
            {   //添加条件定时器(如果对应的共享指针存在，执行定时器回调函数)
                auto t = winfo.lock();  //返回winfo对应的存储空共享指针(timer_info)
                if(!t || t->cancelled)  //没有触发定时器    cancelled判断是否设错误
                {
                    return;
                }
                t->cancelled = ETIMEDOUT;   //ETIMEDOUT超时
                iom->cancelEvent(fd, sylar::IOManager::WRITE);  //超时，则执行函数
            }, winfo);
        }

        int rt = iom->addEvent(fd, sylar::IOManager::WRITE);    //添加IOmanager事件
        if(rt == 0) //添加成功
        {
            sylar::Fiber::YieldToHold();    //将当前协程切换到后台，并将主协程切换出来
            if(timer)
            {
                timer->cancel();    //删除定时器
            }
            if(tinfo->cancelled)
            {
                errno = tinfo->cancelled;
                return -1;
            }
        }
        else
        {
            if(timer)
            {
                timer->cancel();    //删除定时器
            }
            SYLAR_LOG_ERROR(g_logger) << "connect addEvent(" << fd << ", WRITE) error";
        }

        int error = 0;
        socklen_t len = sizeof(int);
        if(-1 == getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &len))    //取出socket状态
        {
            return -1;
        }
        if(!error)
        {
            return 0;
        }
        else
        {
            errno = error;
            return -1;
        }
    }

    int connect(int sockfd,const struct sockaddr* addr,socklen_t addrlen)   //重新编写connect，覆盖原本的系统函数
    {
        return connect_with_timeout(sockfd, addr, addrlen, sylar::s_connect_timeout);   //和connect差不多的函数(增加了超时变量)
    }

    int accept(int s,struct sockaddr* addr,socklen_t* addrlen)  //重新编写accept，覆盖原本的系统函数
    {
        int fd=do_io(s,accept_f,"accept",sylar::IOManager::READ,SO_RCVTIMEO,addr,addrlen);  //有超时时间
        if(fd>=0)
        {
            sylar::FdMgr::GetInstance()->get(fd,true);
        }
        return fd;
    }

    //read
    ssize_t read(int fd, void *buf, size_t count)   //重新编写read，覆盖原本的系统函数
    {
        return do_io(fd,read_f,"read",sylar::IOManager::READ,SO_RCVTIMEO,buf,count);    //SO_RCVTIMEO设置socket接收数据超时时间
    }

    ssize_t readv(int fd, const struct iovec *iov, int iovcnt)  //重新编写readv，覆盖原本的系统函数
    {
        return do_io(fd,readv_f,"readv",sylar::IOManager::READ,SO_RCVTIMEO,iov,iovcnt);
    }

    ssize_t recv(int sockfd, void *buf, size_t len, int flags)  //重新编写recv，覆盖原本的系统函数
    {
        return do_io(sockfd,recv_f,"recv",sylar::IOManager::READ,SO_RCVTIMEO,buf,len,flags);
    }

    ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,struct sockaddr *src_addr, socklen_t *addrlen)    //重新编写recvfrom，覆盖原本的系统函数
    {
        return do_io(sockfd,recvfrom_f,"recvfrom",sylar::IOManager::READ,SO_RCVTIMEO,buf,len,flags,src_addr,addrlen);
    }

    ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags)  //重新编写recvmsg，覆盖原本的系统函数
    {
        return do_io(sockfd,recvmsg_f,"recvmsg",sylar::IOManager::READ,SO_RCVTIMEO,msg,flags);
    }

    //write
    ssize_t write(int fd, const void *buf, size_t count)    //重新编写write，覆盖原本的系统函数
    {
        return do_io(fd,write_f,"write",sylar::IOManager::WRITE,SO_SNDTIMEO,buf,count); //SO_SNDTIMEO设置socket发送数据超时时间
    }

    ssize_t writev(int fd, const struct iovec *iov, int iovcnt) //重新编写writev，覆盖原本的系统函数
    {
        return do_io(fd,writev_f,"writev",sylar::IOManager::WRITE,SO_SNDTIMEO,iov,iovcnt);
    }

    ssize_t send(int sockfd, const void *buf, size_t len, int flags)    //重新编写send，覆盖原本的系统函数
    {
        return do_io(sockfd,send_f,"send",sylar::IOManager::WRITE,SO_SNDTIMEO,buf,len,flags);
    }

    ssize_t sendto(int sockfd, const void *buf, size_t len, int flags,const struct sockaddr *dest_addr, socklen_t addrlen)  //重新编写sendto，覆盖原本的系统函数
    {
        return do_io(sockfd,sendto_f,"sendto",sylar::IOManager::WRITE,SO_SNDTIMEO,buf,len,flags,dest_addr,addrlen);
    }

    ssize_t sendmsg(int sockfd, const struct msghdr *msg, int flags)    //重新编写sendmsg，覆盖原本的系统函数
    {
        return do_io(sockfd,sendmsg_f,"sendmsg",sylar::IOManager::WRITE,SO_SNDTIMEO,msg,flags);
    }

    //close
    int close(int fd)   //重新编写write，覆盖原本的系统函数
    {
        if(!sylar::t_hook_enable)   //当前线程不采用hook
        {
            return close_f(fd);
        }
        sylar::FdCtx::ptr ctx=sylar::FdMgr::GetInstance()->get(fd); //获取文件句柄类FdCtx————自动创建，并插入容器
        if(ctx)
        {
            auto iom=sylar::IOManager::GetThis();
            if(iom)
            {
                iom->cancelAll(fd); //执行所有事件
            }
            sylar::FdMgr::GetInstance()->del(fd);   //删除文件句柄类
        }
    }

    //socket操作(fd) 相关的
    int fcntl(int fd, int cmd, ... /* arg */ )  //重新编写fcntl，覆盖原本的系统函数
    {
        va_list va;
        va_start(va,cmd);   //宏初始化变量
        switch(cmd)
        {
            //int
            case F_SETFL:
                {
                    int arg=va_arg(va,int); //返回可变的参数
                    va_end(va); //清空
                    sylar::FdCtx::ptr ctx=sylar::FdMgr::GetInstance()->get(fd); //获取文件句柄类FdCtx————自动创建，并插入容器
                    if(!ctx || ctx->isClose() || !ctx->isSocket())  //不存在，已关闭，不是socket
                    {
                        return fcntl_f(fd,cmd,arg);
                    }
                    ctx->setSysNonblock(arg & O_NONBLOCK);  //O_NONBLOCK非阻塞
                    if(ctx->getSysNonblock())
                    {
                        arg |= O_NONBLOCK;
                    }
                    return fcntl_f(fd,cmd,arg);
                }
                break;
            case F_DUPFD:
                {
                    va_end(va);
                    int arg = fcntl_f(fd, cmd);
                    sylar::FdCtx::ptr ctx = sylar::FdMgr::GetInstance()->get(fd);   //获取文件句柄类FdCtx————自动创建，并插入容器
                    if(!ctx || ctx->isClose() || !ctx->isSocket())  //不存在，已关闭，不是socket
                    {
                        return arg;
                    }
                    if(ctx->getUserNonblock()) {
                        return arg | O_NONBLOCK;
                    } else {
                        return arg & ~O_NONBLOCK;
                    }
                }
                break;
            case F_DUPFD_CLOEXEC:
            case F_SETFD:
            case F_SETOWN:
            case F_SETSIG:
            case F_SETLEASE:
            case F_NOTIFY:
            case F_SETPIPE_SZ: 
                {
                    int arg=va_arg(va,int); //返回可变的参数
                    va_end(va); //清空
                    return fcntl_f(fd,cmd,arg);
                }           
                break;

            //void
            case F_GETFD:
            case F_GETFL:
            case F_GETOWN:
            case F_GETSIG:
            case F_GETLEASE:
            case F_GETPIPE_SZ:
                {
                    va_end(va); //清空
                    return fcntl_f(fd,cmd);   
                }
                break;

            //struct flock *
            case F_SETLK:
            case F_SETLKW:
            case F_GETLK:
                {
                    struct flock* arg=va_arg(va,struct flock*); //返回可变的参数
                    va_end(va); //清空
                    return fcntl_f(fd,cmd,arg);
                }
                break;

            //struct f_owner_ex *
            case F_GETOWN_EX:
            case F_SETOWN_EX:
                {
                    struct f_owner_exlock* arg=va_arg(va,struct f_owner_exlock*);   //返回可变的参数
                    va_end(va); //清空
                    return fcntl_f(fd,cmd,arg);
                }
                break;
            default:
                va_end(va); //清空
                return fcntl_f(fd,cmd); 
        }
    }

    int ioctl(int d, unsigned long int request, ...)  //重新编写ioctl，覆盖原本的系统函数(ioctl控制接口函数)
    {
        va_list va;
        va_start(va, request);  //宏初始化变量
        void* arg = va_arg(va, void*);  //返回可变的参数
        va_end(va); //清空

        if(FIONBIO == request)
        {
            bool user_nonblock = !!*(int*)arg;  //是否阻塞
            sylar::FdCtx::ptr ctx = sylar::FdMgr::GetInstance()->get(d);    //获取文件句柄类FdCtx————自动创建，并插入容器
            if(!ctx || ctx->isClose() || !ctx->isSocket())  //不存在，已关闭，不是socket
            {
                return ioctl_f(d, request, arg);
            }
            ctx->setUserNonblock(user_nonblock);
        }
        return ioctl_f(d, request, arg);
    }

    int getsockopt(int sockfd, int level, int optname,void *optval, socklen_t *optlen)  //重新编写getsockopt，覆盖原本的系统函数(getsockopt获取一个套接字的选项)
    {
        return getsockopt_f(sockfd, level, optname, optval, optlen);
    }

    int setsockopt(int sockfd, int level, int optname,const void *optval, socklen_t optlen) //为网络套接字设置选项值，比如：允许重用地址、网络超时 
    {
        if(!sylar::t_hook_enable)   //当前线程不采用hook
        {
            return setsockopt_f(sockfd, level, optname, optval, optlen);
        }
        if(level == SOL_SOCKET)
        {
            if(optname == SO_RCVTIMEO || optname == SO_SNDTIMEO)    //SO_RCVTIMEO接受超时   SO_SNDTIMEO发送超时
            {
                sylar::FdCtx::ptr ctx = sylar::FdMgr::GetInstance()->get(sockfd);   //获取文件句柄类FdCtx————自动创建，并插入容器
                if(ctx)
                {
                    const timeval* v = (const timeval*)optval;
                    ctx->setTimeout(optname, v->tv_sec * 1000 + v->tv_usec / 1000); //设置超时时间——毫秒
                }
            }
        }
        return setsockopt_f(sockfd, level, optname, optval, optlen);
    }
    // 重新编写setsockopt，覆盖原本的系统函数(setsockopt给网络套接字设置选项值)
    // sockfd：网络套接字
    // level：协议层，整个网络协议中存在很多层，指定由哪一层解析；通常是SOL_SOCKET，也有IPPROTO_IP/IPPROTO_TCP
    // optname：需要操作的选项，比如SO_RCVTIMEO接受超时
    // optval：设置的选项值，类型不定；比如SO_REUSERADDR设置地址重用，那么只需要传入一个int指针即可
    // optlen：选项值的长度
    
}