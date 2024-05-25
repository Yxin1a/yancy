//      协程调度器封装(线程池)
//协程调度器— 1-N —>线程— 1-M —>协程
//                  N     -     M
//  (主协程和子协程来回切换)
//协程调度器负责协程在线程(之中，之间)的切换
/*
    1.线程池，分配一组线程
    2.协程调度器，将协程指定到相应的线程上去执行

    (调度线程池中有空闲线程就调用协程队列中的协程)

    (*自动调度线程池中的协程切换，还有状态转换*)
*/

#ifndef __SYLAR_SCHEDULER_H__
#define __SYLAR_SCHEDULER_H__

#include<memory>
#include<vector>
#include<list>
#include<iostream>
#include"fiber.h"
#include"thread.h"

namespace sylar
{
    
    /**
     *  协程调度器
     *      封装的是N-M的协程调度器
     *      内部有一个线程池,支持协程在线程池里面切换
     */
    class Scheduler
    {
    public:
        typedef std::shared_ptr<Scheduler> ptr;
        typedef Mutex MutexType;
        
        /**
         *  构造函数
         *      threads 线程数量
         *      use_caller 是否使用当前调用线程
         *      name 协程调度器名称
         */
        Scheduler(size_t threads=1,bool use_caller=true,const std::string& name="");

        /**
         *  析构函数
         */
        virtual ~Scheduler();

        const std::string& getName() const {return m_name;} //返回协程调度器名称

        static Scheduler* GetThis();    //返回当前协程调度器
        static Fiber* GetMainFiber();   //返回当前协程调度器的调度协程(主协程)

        void start();   //启动协程调度器
        void stop();    //停止协程调度器

        /**
         *  调度协程
         *      fc 协程或函数
         *      thread 协程执行的线程id,-1标识任意线程
         *      (协程队列插入)
         */
        template<class FiberOrCb>
        void schedule(FiberOrCb fc,int thread=-1)
        {
            bool need_tickle=false;
            {
                MutexType::Lock lock(m_mutex);  //互斥量——m_fibers
                need_tickle=scheduleNoLock(fc,thread);  //查看协程调度器是否有任务
            }
            if(need_tickle)
            {
                tickle();   //通知协程调度器有任务了
            }
        }

        /**
         *  批量调度协程
         *      begin 协程数组的开始
         *      end 协程数组的结束
         *      (协程队列批量插入)
         */
        template<class InputIterator>
        void schedule(InputIterator begin,InputIterator end)
        {
            bool need_tickle=false;
            {
                MutexType::Lock lock(m_mutex);
                while (begin!=end)  //遍历协程数组
                {
                    need_tickle=scheduleNoLock(&*begin,-1) || need_tickle; //查看协程调度器是否有任务
                    ++begin;
                }
            }
            if(need_tickle)
            {
                tickle();   //通知协程调度器有任务了
            }
        }

        /**
         * @brief 切换所在的协程调度器为当前协程调度器，并使用thread处理协程
         * @param[in] thread 线程id
        */
        void switchTo(int thread = -1);

        /**
         * @brief 输出协程调度器基本信息到流中
        */
        std::ostream& dump(std::ostream& os);

    protected:
        virtual void tickle();  //通知协程调度器有任务

        void run();     //协程调度函数，创建idle、子协程，并执行子协程函数

        virtual bool stopping();    //返回是否已经停止

        void setThis(); //设置为当前的协程调度器

        virtual void idle();    //协程无任务可调度、线程不能停止时执行idle协程(所有任务已完成)

        bool hasIdleThreads() { return m_idleThreadCount>0; }   //获取存在空闲线程
    private:
        /**
         *  协程调度启动(无锁)
         *      1.查看协程调度器是否有任务
         *      2.并插入数据给协程队列
         */
        template<class FiberOrCb>
        bool scheduleNoLock(FiberOrCb fc,int thread)
        {
            bool need_tickle=m_fibers.empty();  //协程队列是否为空
            FiberAndThread ft(fc,thread);
            if(ft.fiber || ft.cb)   //协程是否存在
            {
                m_fibers.push_back(ft); //插入
            }
            return need_tickle;
        }

    private:
        /**
         *  协程/执行函数/线程组
         *      协程队列包括的数据
         *      (协程/执行函数二选一————已创建协程/有执行函数，但是未创建协程)
         */
        struct FiberAndThread
        {
            Fiber::ptr fiber;   //协程
            std::function<void()> cb;   //协程执行函数
            int thread;     //线程id

            /**
             *  构造函数
             *      f 协程
             *      thr 线程id
             */
            FiberAndThread(Fiber::ptr f,int thr):fiber(f),thread(thr){}

            /**
             *  构造函数
             *      f 协程指针
             *      thr 线程id
             *      *f = nullptr
             */
            FiberAndThread(Fiber::ptr* f,int thr):thread(thr)
            {
                fiber.swap(*f); //swap()交换两个shared_ptr对象(即交换所拥有的对象)
            }
            
            /**
             *  构造函数
             *      f 协程执行函数
             *      thr 线程id
             */
            FiberAndThread(std::function<void()> f,int thr):cb(f),thread(thr){}

            /**
             *  构造函数
             *      f 协程执行函数指针
             *      thr 线程id
             *      *f = nullptr
             */
            FiberAndThread(std::function<void()>* f,int thr):thread(thr)
            {
                cb.swap(*f);    //swap()交换两个可调用函数的对象。
            }

            /**
             *  无参默认构造函数
             */
            FiberAndThread():thread(-1){}   //STL分配对象的时候必须有一个默认构造函数(不然分配的对象无法初始化)
                                            //(STL自定义类型有默认构造函数)

            /**
             *  重置数据
             */
            void reset()
            {
                fiber=nullptr;
                cb=nullptr;
                thread=-1;
            }
        };

    private:
        MutexType m_mutex;  //互斥量（读写共享数据才要加锁——std::list<FiberAndThread>——std::vector<Thread::ptr>）
        std::vector<Thread::ptr> m_threads;  //线程池
        std::list<FiberAndThread> m_fibers;   //待执行的协程队列(包括function<void()>、fiber、threadid)
        Fiber::ptr m_rootFiber; //主协程(use_caller为true时有效, 调度协程)
        std::string m_name; //协程调度器名称

    protected:
        //协程状态下线程的情况
        std::vector<int> m_threadIds;   //协程下的线程id数组
        size_t m_threadCount=0;           //线程数量
        std::atomic<size_t> m_activeThreadCount= {0};     //工作线程数量    (atomic变量不支持拷贝)
        std::atomic<size_t> m_idleThreadCount= {0};       //空闲线程数量    (atomic变量不支持拷贝)
        bool m_stopping=true;    //是否已经停止
        bool m_autoStop=false;    //是否自动停止
        int m_rootThread=0;   //主线程id(use_caller)
    };

    /**
     * @brief Scheduler切换器
     *      协程调度器的切换器
    */
    class SchedulerSwitcher : public Noncopyable
    {
    public:
        /**
         * @brief 切换协程调度器
         * @param[out] target 要切换到的协程调度器 
        */
        SchedulerSwitcher(Scheduler* target = nullptr);
        /**
         * @brief 切换回原来的协程调度器
        */
        ~SchedulerSwitcher();
    private:
        /// 原来的协程调度器
        Scheduler* m_caller;
    };

}

#endif