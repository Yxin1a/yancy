//一个文件描述符管理多个文件描述符
//使用一对管道 fd 来 tickle 调度协程
//epoll
//epoll树存放————读、写 事件
//epoll_wait检测    socket句柄是否有    连接——写事件    服务器读——读事件
//triggerEvent()插入协程队列
//idle()检测、处理事件
//取消事件====插入协程队列
//父类的run()执行事件对应的执行函数
#include"iomanager.h"
#include<sys/epoll.h>
#include<unistd.h>
#include<fcntl.h>
#include<errno.h>
#include<string.h>

#include "macro.h"
#include "log.h"

namespace sylar
{
    static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

    IOManager::FdContext::EventContext& IOManager::FdContext::getContext(IOManager::Event event)    //获取事件上下文类(返回对应事件的上线文类)
    {
        switch (event)
        {
        case IOManager::READ:
            return read;
        case IOManager::WRITE:
            return write;
        default:
            SYLAR_ASSERT2(false,"getContext");
        }
    }

    void IOManager::FdContext::resetContext(EventContext& ctx)     //重置事件上下文,ctx待重置的上下文类
    {
        ctx.scheduler=nullptr;
        ctx.fiber.reset();
        ctx.cb=nullptr;
    }

    void IOManager::FdContext::triggerEvent(IOManager::Event event)        //触发事件(插入协程队列),event 事件类型
    {
        SYLAR_ASSERT(events & event);   //(0x1 & 0x1)和(0x4 & 0x4)  >0
        events=(Event)(events & ~event);    //~是取反的意思 ~0x0=-1 ~0x1=-2 ~0x4=-1
        EventContext& ctx=getContext(event);
        if(ctx.cb)
        {
            ctx.scheduler->schedule(&ctx.cb);
        }
        else
        {
            ctx.scheduler->schedule(&ctx.fiber);
        }
        ctx.scheduler=nullptr;
        return;
    }

    IOManager::IOManager(size_t threads,bool use_caller,const std::string& name)    //初始化，创建epoll、管道节点，添加epoll实例数据
        :Scheduler(threads,use_caller,name)     //在子类构造函数创建父类    Scheduler
    {
        m_epfd=epoll_create(5000);  //epoll_create()创建一个epoll树(红黑树————存事件)
                                    //参数：>0的整数
                                    //返回值：返回一个返回文件描述符(树根节点)
        SYLAR_ASSERT(m_epfd>0);
        
        int rt=pipe(m_tickleFds);   //pipe管道—进程通信 pipe()创建,并打开管道   成功：0； 失败：-1
        SYLAR_ASSERT(!rt);

        epoll_event event;
        memset(&event,0,sizeof(epoll_event));   //清零————将event中当前位置后面的sizeof(epoll_event)个字节用0替换并返回event
        event.events=EPOLLIN | EPOLLET; //events事件是int类型，EPOLLET是EPOLLIN其中的一种读方式————EPOLLIN：可读事件；EPOLLOUT：可写事件；EPOLLERR：异常事件。
        event.data.fd=m_tickleFds[0];   //委托内核监控的文件描述符————events的备注信息——fd(管道读写端是文件描述符)

        rt=fcntl(m_tickleFds[0],F_SETFL,O_NONBLOCK);    //经常用这个fcntl函数对已打开的文件描述符改变为非阻塞
        SYLAR_ASSERT(!rt);

        rt=epoll_ctl(m_epfd,EPOLL_CTL_ADD,m_tickleFds[0],&event);   //对文件描述符m_epfd引用的文件描述符(节点)执行控制操作(添加、修改或者删除)
                                                                    //参数：epoll实例的句柄
                                                                    //      控制操作
                                                                    //      要操作的文件描述符(节点)
                                                                    //      文件描述符对应的事件(修改，删除)
                                                                    //成功：0； 失败：-1
        SYLAR_ASSERT(!rt);

        contextResize(32);

        start();
    }
        
    IOManager::~IOManager() //关闭协程调度器，关闭句柄，释放socket句柄容器内存
    {
        stop();
        close(m_epfd);
        close(m_tickleFds[0]);
        close(m_tickleFds[1]);

        for(size_t i=0;i<m_fdContexts.size();++i)
        {
            if(m_fdContexts[i])
            {
                delete m_fdContexts[i];
            }
        }
    }

    void IOManager::contextResize(size_t size)  //重置socket句柄上下文的容器大小
    {
        m_fdContexts.resize(size);

        for(size_t i=0;i<m_fdContexts.size();++i)   //容器的下标=句柄
        {
            if(!m_fdContexts[i])
            {
                m_fdContexts[i]=new FdContext;
                m_fdContexts[i]->fd=i;
            }
        }
    }

    bool IOManager::stopping(uint64_t& timeout)    //判断是否可以停止
    {
        timeout=getNextTimer();
        return timeout==~0ull
            && m_pendingEventCount==0
            && Scheduler::stopping();
    }

    int IOManager::addEvent(int fd,Event event,std::function<void()> cb)   //添加事件
    {
        FdContext* fd_ctx=nullptr;
        RWMutexType::ReadLock lock(m_mutex);
        if((int)m_fdContexts.size()>fd)  //容器的大小放得下该句柄
        {
            fd_ctx=m_fdContexts[fd];
            lock.unlock();
        }
        else    //容器的大小放不下该句柄
        {
            lock.unlock();
            RWMutexType::WriteLock lock2(m_mutex);
            contextResize(fd*1.5);
            fd_ctx=m_fdContexts[fd];
        }

        FdContext::MutexType::Lock lock2(fd_ctx->mutex);
        if(fd_ctx->events & event)  //&是按位与————只要有0就是0,两个都是1才是1  (0x1 & 0x1)和(0x4 & 0x4)  >0
        {
            SYLAR_LOG_ERROR(g_logger)<<"addEvent assert fd="<<fd
                        <<" event="<<event
                        <<" fd_ctx.event="<<fd_ctx->events;
            SYLAR_ASSERT(!(fd_ctx->events & event));
        }

        int op=fd_ctx->events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;  //1则修改，2则添加
        epoll_event epevent;
        epevent.events=EPOLLET | fd_ctx->events | event;    //events事件是int类型  fd_ctx->events | event一方有事件，则有事件
        epevent.data.ptr=fd_ctx;    //委托内核监控的文件描述符————events存储用户数据（ptr）

        int rt=epoll_ctl(m_epfd,op,fd,&epevent);    //对文件描述符m_epfd引用的文件描述符(节点)执行控制操作(添加、修改或者删除)
                                                    //参数：epoll实例的句柄
                                                    //      控制操作
                                                    //      要操作的文件描述符(节点)
                                                    //      文件描述符对应的事件(修改，删除)
                                                    //成功：0； 失败：-1
        if(rt)  //失败
        {
            SYLAR_LOG_ERROR(g_logger)<<"epoll_ctl("<<m_epfd<<", "
                    <<op<<","<<fd<<","<<epevent.events<<"):"
                    <<rt<<" ("<<errno<<") ("<<strerror(errno)<<")";
            return -1;
        }

        ++m_pendingEventCount;
        fd_ctx->events=(Event)(fd_ctx->events | event);
        FdContext::EventContext& event_ctx=fd_ctx->getContext(event);
        SYLAR_ASSERT(!event_ctx.scheduler
                    && !event_ctx.fiber
                    && !event_ctx.cb);

        event_ctx.scheduler=Scheduler::GetThis();
        if(cb)
        {
            event_ctx.cb.swap(cb);
        }
        else
        {
            event_ctx.fiber=Fiber::GetThis();
            SYLAR_ASSERT(event_ctx.fiber->getState()==Fiber::EXEC);
        }

        return 0;
    }
    
    bool IOManager::delEvent(int fd,Event event)   //删除事件
    {
        RWMutexType::ReadLock lock(m_mutex);
        if((int)m_fdContexts.size()<=fd)
        {
            return false;
        }
        FdContext* fd_ctx=m_fdContexts[fd];
        lock.unlock();

        FdContext::MutexType::Lock lock2(fd_ctx->mutex);
        if(!(fd_ctx->events & event))   //&是按位与————只要有0就是0,两个都是1才是1  (0x1 & 0x1)和(0x4 & 0x4)  >0
        {
            return false;
        }

        Event new_events=(Event)(fd_ctx->events & ~event);
        int op=new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;  //无事件是修改，有事件是删除
        epoll_event epevent;
        epevent.events=EPOLLET | new_events;    //events事件是int类型
        epevent.data.ptr=fd_ctx;    //委托内核监控的文件描述符————events存储用户数据（ptr）

        int rt=epoll_ctl(m_epfd,op,fd,&epevent);    //对文件描述符m_epfd引用的epoll实例执行控制操作(添加、修改或者删除)
                                                    //参数：epoll实例的句柄
                                                    //      控制操作
                                                    //      要操作的文件描述符(节点)
                                                    //      文件描述符对应的事件(修改，删除)
                                                    //成功：0； 失败：-1
        if(rt)  //失败
        {
            SYLAR_LOG_ERROR(g_logger)<<"epoll_ctl("<<m_epfd<<", "
                    <<op<<","<<fd<<","<<epevent.events<<"):"
                    <<rt<<" ("<<errno<<") ("<<strerror(errno)<<")";
            return false;
        }

        --m_pendingEventCount;
        fd_ctx->events=new_events;
        FdContext::EventContext& event_ctx=fd_ctx->getContext(event);
        fd_ctx->resetContext(event_ctx);
        return false;
    }

    bool IOManager::cancelEvent(int fd,Event event) //取消事件(取消的时候回执行一段函数————取消=执行)
    {
        RWMutexType::ReadLock lock(m_mutex);
        if((int)m_fdContexts.size()<=fd)
        {
            return false;
        }
        FdContext* fd_ctx=m_fdContexts[fd];
        lock.unlock();

        FdContext::MutexType::Lock lock2(fd_ctx->mutex);
        if(!(fd_ctx->events & event))   //&是按位与————只要有0就是0,两个都是1才是1  (0x1 & 0x1)和(0x4 & 0x4)  >0
        {
            return false;
        }

        Event new_events=(Event)(fd_ctx->events & ~event);
        int op=new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;  //无事件是修改，有事件是删除
        epoll_event epevent;
        epevent.events=EPOLLET | new_events;    //events事件是int类型
        epevent.data.ptr=fd_ctx;    //委托内核监控的文件描述符————events存储用户数据（ptr）

        int rt=epoll_ctl(m_epfd,op,fd,&epevent);    //对文件描述符m_epfd引用的文件描述符(节点)执行控制操作(添加、修改或者删除)
                                                    //参数：epoll实例的句柄
                                                    //      控制操作
                                                    //      要操作的文件描述符(节点)
                                                    //      文件描述符对应的事件(修改，删除)
                                                    //成功：0； 失败：-1
        if(rt)  //失败
        {
            SYLAR_LOG_ERROR(g_logger)<<"epoll_ctl("<<m_epfd<<", "
                    <<op<<","<<fd<<","<<epevent.events<<"):"
                    <<rt<<" ("<<errno<<") ("<<strerror(errno)<<")";
            return false;
        }

        fd_ctx->triggerEvent(event);
        --m_pendingEventCount;
        return false;
    }

    bool IOManager::cancelAll(int fd)  //取消所有事件(取消的时候回执行一段函数————取消=执行)
    {
        RWMutexType::ReadLock lock(m_mutex);
        if((int)m_fdContexts.size()<=fd)
        {
            return false;
        }
        FdContext* fd_ctx=m_fdContexts[fd];
        lock.unlock();

        FdContext::MutexType::Lock lock2(fd_ctx->mutex);
        if(!fd_ctx->events)
        {
            return false;
        }

        int op=EPOLL_CTL_DEL;
        epoll_event epevent;
        epevent.events=0;
        epevent.data.ptr=fd_ctx;    //委托内核监控的文件描述符————events存储用户数据（ptr）

        int rt=epoll_ctl(m_epfd,op,fd,&epevent);    //对文件描述符m_epfd引用的文件描述符(节点)执行控制操作(添加、修改或者删除)
                                                    //参数：epoll实例的句柄
                                                    //      控制操作
                                                    //      要操作的文件描述符(节点)
                                                    //      文件描述符对应的事件(修改，删除)
                                                    //成功：0； 失败：-1
        if(rt)  //失败
        {
            SYLAR_LOG_ERROR(g_logger)<<"epoll_ctl("<<m_epfd<<", "
                    <<op<<","<<fd<<","<<epevent.events<<"):"
                    <<rt<<" ("<<errno<<") ("<<strerror(errno)<<")";
            return false;
        }

        if(fd_ctx->events & READ)   //判断相等
        {
            fd_ctx->triggerEvent(READ);
            --m_pendingEventCount;
        }
        
        if(fd_ctx->events & WRITE)  //判断相等
        {
            fd_ctx->triggerEvent(WRITE);
            --m_pendingEventCount;
        }
        
        SYLAR_ASSERT(fd_ctx->events==0);
        return false;
    }

    IOManager* IOManager::GetThis() //返回当前的IOManager
    {
        return dynamic_cast<IOManager*>(Scheduler::GetThis());  //父类和子类之间的强制类型转换
    }

    void IOManager::tickle()       //通知协程调度器有任务
    {
        if(!hasIdleThreads())
        {
            return;
        }
        int rt=write(m_tickleFds[1],"T",1); //服务器发到客户端
        SYLAR_ASSERT(rt==1);
    }

    bool IOManager::stopping()     //返回是否已经停止
    {
        uint64_t timeout=0;
        return stopping(timeout);
    }

    void IOManager::idle()         //协程无任务可调度，(检测epoll树是否有就绪(触发事件)的节点,且执行节点对应的函数————只检测一次)   //插入协程队列定时器的执行函数
    {
        epoll_event* events=new epoll_event[64]();
        std::shared_ptr<epoll_event> shared_events(events,[](epoll_event* ptr) {    delete[] ptr;   });

        while (true)
        {
            uint64_t next_timeout=0;    //存储定时器的执行时间
            if(stopping(next_timeout))  //返回是否已经停止
            {
                SYLAR_LOG_INFO(g_logger)<<"name="<<getName()
                                    <<" idle stopping exit";
                break;
            }

            int rt=0;
            do  //检测epoll树中是否有就绪的文件描述符
            {
                static const int MAX_TIMEOUT=3000;  //最小超时器——1000毫秒
                
                if(next_timeout!=~0ull)
                {
                    next_timeout=(int)next_timeout>MAX_TIMEOUT ? MAX_TIMEOUT : next_timeout;
                }
                else
                {
                    next_timeout=MAX_TIMEOUT;
                }
                rt=epoll_wait(m_epfd,events,64,(int)next_timeout);    //检测epoll树中是否有就绪的文件描述符(阻塞)————只能检测一次
                                                                //参数：epoll树根节点
                                                                //      epoll_event结构体数组(存放就绪文件描述符)
                                                                //      数组大小
                                                                //      阻塞的时长
                                                                //返回值：就绪文件描述符个数

                if(rt<0 && errno==EINTR)    //EINTR数据返回时，尝试中断了
                {}
                else
                {
                    break;
                }
            } while (true);
            
            std::vector<std::function<void()>> cbs;
            listExpiredCb(cbs);
            if(!cbs.empty())    //定时器有执行函数就插入协程队列中
            {
                schedule(cbs.begin(),cbs.end());
                cbs.clear();
            }

            for(int i=0;i<rt;++i)   //处理就绪的文件描述符
            {
                epoll_event& event=events[i];
                if(event.data.fd==m_tickleFds[0])   //处理外部有发消息来(触发读事件fd)，要接受
                {
                    uint8_t dummy;
                    while (read(m_tickleFds[0],&dummy,1)==1);
                    continue;
                }

                FdContext* fd_ctx=(FdContext*)event.data.ptr;   //处理ptr的数据
                FdContext::MutexType::Lock lock(fd_ctx->mutex);
                if(event.events & (EPOLLERR | EPOLLHUP))    //EPOLLERR和EPOLLHUP====>EPOLLIN和EPOLLOUT
                {
                    event.events |= EPOLLIN | EPOLLOUT;     //  A|=B  ————> A=A|B   
                }
                int real_events=NONE;
                if(event.events & EPOLLIN)  //判断是否是EPOLLIN：可读事件
                {
                    real_events |= READ;    //real_events=READ
                }
                if(event.events & EPOLLOUT) //判断是否是EPOLLOUT：可写事件
                {
                    real_events |= WRITE;   //real_events=WRITE
                }

                if((fd_ctx->events & real_events)==NONE)    //没有事件  //fd_ctx->events & real_events判断是否相等
                {
                    continue;
                }

                //处理事件
                int left_events=(fd_ctx->events & ~real_events);    //~1=-2 ~4=-5   1&-2=0 4&-5=0
                int op=left_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
                event.events=EPOLLET | left_events;

                int rt2=epoll_ctl(m_epfd,op,fd_ctx->fd,&event);     //对文件描述符m_epfd引用的文件描述符(节点)执行控制操作(添加、修改或者删除)
                                                                    //参数：epoll实例的句柄
                                                                    //      控制操作
                                                                    //      要操作的文件描述符(节点)
                                                                    //      文件描述符对应的事件(修改，删除)
                                                                    //成功：0； 失败：-1
                if(rt2)  //失败
                {
                    SYLAR_LOG_ERROR(g_logger)<<"epoll_ctl("<<m_epfd<<", "
                            <<op<<","<<fd_ctx->fd<<","<<event.events<<"):"
                            <<rt2<<" ("<<errno<<") ("<<strerror(errno)<<")";
                    continue;
                }

                if(real_events & READ)
                {
                    fd_ctx->triggerEvent(READ);
                    --m_pendingEventCount;
                }
                if(real_events & WRITE)
                {
                    fd_ctx->triggerEvent(WRITE);
                    --m_pendingEventCount;
                }
            }
            //把idle协程控制权让到协程调度框架里去
            Fiber::ptr cur=Fiber::GetThis();
            auto raw_ptr=cur.get();
            cur.reset();

            raw_ptr->swapOut();
        }
        
    }

    void IOManager::onTimerInsetedAtFront()   //当有新的定时器插入到定时器的首部,执行该函数
    {
        tickle();
    }

}