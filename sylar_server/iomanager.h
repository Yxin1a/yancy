//  基于Epoll的IO协程调度器
//继承协程调度器，拥有协程调度器的功能
//通过定时器管理，来管理定时器(IO协程调度器有自己的等待时间，和定时器没联系)
#ifndef __SYLAR_IOMANAGER_H__
#define __SYLAR_IOMANAGER_H__

#include"scheduler.h"
#include"timer.h"

namespace sylar
{

    /**
     *  基于Epoll的IO协程调度器
     */
    class IOManager: public Scheduler,public TimerManager
    {
    public:
        typedef std::shared_ptr<IOManager> ptr;
        typedef RWMutex RWMutexType;

        /**
         *  IO事件
         */
        enum Event
        {
            NONE=0x0,   //无事件
            READ=0x1,   //读事件(EPOLLIN=1)
            WRITE=0x4   //写事件(EPOLLOUT=4)
        };
    
    private:

        /**
         *  Socket事件上线文类(协程调度器)————一个资源读写事件函数
         *      (event.data.ptr指向)
         */
        struct FdContext
        {
            typedef Mutex MutexType; 

            /**
             *  事件上线文类(一次事件)
             *      fiber，cb回调函数(二选一)
             */
            struct EventContext
            {
                Scheduler* scheduler=nullptr;   //事件执行的调度器
                Fiber::ptr fiber;               //事件协程
                std::function<void()> cb;       //事件的回调函数
            };

            /**
             *  获取事件上下文类
             *      event 事件类型
             *      返回对应事件的上线文
             */
            EventContext& getContext(Event event);

            /**
             *  重置事件上下文
             *      ctx 待重置的上下文类
             */
            void resetContext(EventContext& ctx);

            /**
             *  触发事件(插入协程队列)
             *      event 事件类型
             */
            void triggerEvent(Event event);
 
            EventContext read;  //读事件上下文
            EventContext write; //写事件上下文
            int fd=0;           //事件关联的句柄
            Event events=NONE;  //当前的事件
            MutexType mutex;    //事件的Mutex
        };

    public:

        /**
         *  构造函数
         *      threads 线程数量
         *      use_caller 是否将调用线程包含进去
         *      name 调度器的名称
         */
        IOManager(size_t threads=1,bool use_caller=true,const std::string& name="");
        
        /**
         *  析构函数
         */
        ~IOManager();

        /**
         *  添加事件
         *      fd socket句柄
         *      event 事件类型
         *      cb 事件回调函数
         *      添加成功返回0,失败返回-1
         */
        int addEvent(int fd,Event event,std::function<void()> cb=nullptr);
        
        /**
         *  删除事件
         *      fd socket句柄
         *      event 事件类型
         *      不会触发事件
         */
        bool delEvent(int fd,Event event);

        /**
         *  取消事件(取消的时候回执行一段函数————取消=执行)
         *      fd socket句柄
         *      event 事件类型
         *      如果事件存在则触发事件
         */
        bool cancelEvent(int fd,Event event);

        /**
         *  取消所有事件
         *      fd socket句柄
         */
        bool cancelAll(int fd);

        /**
         *  返回当前的IOManager
         */
        static IOManager* GetThis();

    protected:
        void tickle() override; //通知协程调度器有任务
        bool stopping() override;   //返回是否已经停止
        void idle() override;   //协程无任务可调度、线程不能停止时执行idle协程(所有任务已完成)

        void onTimerInsetedAtFront() override;   //当有新的定时器插入到定时器的首部,执行该函数

        /**
         *  重置socket句柄上下文的容器大小
         *      size 容量大小
         */
        void contextResize(size_t size);

        /**
         *  判断是否可以停止
         *      timeout 最近要触发的定时器事件间隔
         *      返回是否可以停止
         */
        bool stopping(uint64_t& timeout);
        
    private:
        int m_epfd=0;   //epoll文件句柄
        int m_tickleFds[2]; //pipe文件句柄  0为读——1为写

        std::atomic<size_t> m_pendingEventCount={0};    //当前等待执行的事件数量
        RWMutexType m_mutex;    //IOManager的Mutex
        std::vector<FdContext*> m_fdContexts;   //socket事件上下文的容器    容器的下标=句柄
    };  

}

#endif  