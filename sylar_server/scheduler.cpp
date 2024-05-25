//协程调度器原理:
//(1)使用当前调用线程
/*
    根协程(当前调用线程)①————————>主协程run②————————>子协程idle③(判断结束)
              线程池的线程——1:1——          —————————>子协程(协程队列的协程)④
    （线程>协程     线程池创建每一条以协程run为执行函数的线程）

//切换：
①——>②    ②——>④    ④——>②    ②——>③    ③——>②    删④ 删③ 删② 删①
*/

//(2)不使用当前调用线程
/*
    线程run①——1:1————>主协程run②————————>子协程idle③(判断结束)
                           —————————>子协程(协程队列的协程)④
    （线程>协程     线程池创建每一条以协程run为执行函数的线程）

//切换：
②——>④    ④——>②    ②——>③    ③——>②    删④ 删③ 删②
*/
#include"scheduler.h"
#include"log.h"
#include"macro.h"
#include"hook.h"

namespace sylar
{
    static sylar::Logger::ptr g_logger=SYLAR_LOG_NAME("system");

    //thread_local————t_scheduler仅可在thread_local上创建的协程上访问。用于协程本地对象的声明
    static thread_local Scheduler* t_scheduler=nullptr;     //当前协程调度器指针
    static thread_local Fiber* t_fiber=nullptr; //当前协程调度器的调度协程(主协程)

    Scheduler::Scheduler(size_t threads,bool use_caller,const std::string& name):m_name(name)   //初始化
    {
        SYLAR_ASSERT(threads>0);    //线程数量大于0
        
        if(use_caller)      //是否使用当前调用线程
        {
            sylar::Fiber::GetThis();    //返回当前所在的协程，没有则创建新的主协程(无执行函数)
            --threads;

            SYLAR_ASSERT(GetThis()==nullptr);
            t_scheduler=this;

            m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run,this),0,true));  //主协程存协程调度器的执行函数(创建子协程)
            sylar::Thread::SetName(m_name);

            t_fiber=m_rootFiber.get();  //get()，返回内部对象(指针)
            m_rootThread=sylar::GetThreadId();
            m_threadIds.push_back(m_rootThread);
        }
        else
        {
            m_rootThread=-1;
        }
        m_threadCount=threads;
    }

    Scheduler::~Scheduler() //释放
    {
        SYLAR_ASSERT(m_stopping);
        if(GetThis()==this)
        {
            t_scheduler=nullptr;
        }
    }

    Scheduler* Scheduler::GetThis()    //返回当前协程调度器
    {
        return t_scheduler;
    }
    
    Fiber* Scheduler::GetMainFiber()   //返回当前协程调度器的调度协程(主协程)
    {
        return t_fiber;
    }

    void Scheduler::start()   //启动协程调度器
    {
        MutexType::Lock lock(m_mutex);
        if(!m_stopping)
        {
            return;
        }
        m_stopping=false;
        SYLAR_ASSERT(m_threads.empty()); //启动时，线程池只能为空

        m_threads.resize(m_threadCount); //resize()重新定义vector的大小
        for(size_t i=0;i<m_threadCount;++i) //重新设置线程池
        {
            m_threads[i].reset(new Thread(std::bind(&Scheduler::run,this)    //将可调用对象(函数)和参数一起绑定，绑定后的结果使用std::function进行保存
                                ,m_name+"_"+std::to_string(i)));    //顺便创建线程执行协程中run执行函数

            m_threadIds.push_back(m_threads[i]->getId());
        }

        lock.unlock();  //没有则死锁了
    }

    void Scheduler::stop()    //停止协程调度器
    {
        m_autoStop=true;
        //m_rootFiber && m_threadCount==0 && (m_rootFiber->getState()==Fiber::TERM || m_rootFiber->getState()==Fiber::INIT)
        if(m_rootFiber
                && m_threadCount==0
                && (m_rootFiber->getState()==Fiber::TERM
                    || m_rootFiber->getState()==Fiber::INIT))   //判断是否符合停止协程调度器的条件
        {
            SYLAR_LOG_INFO(g_logger)<<this<<" stopped";
            m_stopping=true;

            if(stopping())
            {
                return;
            }
        }

        //不能停止的原因
        // bool exit_on_this_fiber=false;
        if(m_rootThread!=-1)
        {
            SYLAR_ASSERT(GetThis()==this);
        }
        else
        {
            SYLAR_ASSERT(GetThis()!=this);
        }

        m_stopping=true;
        for(size_t i=0;i<m_threadCount;++i)
        {
            tickle();   //通知协程调度器有任务(唤醒)
        }

        if(m_rootFiber)
        {
            tickle();   //通知协程调度器有任务(唤醒)
        }

        if(m_rootFiber)
        {   
            if(!stopping())
            {
                m_rootFiber->call();
            }
        }

        std::vector<Thread::ptr> thrs;
        {
            MutexType::Lock lock(m_mutex);
            thrs.swap(m_threads);   //交换vector元素
        }

        for(auto& i:thrs)
        {
            i->join();  //等待线程执行完成
        }
    }

    void Scheduler::setThis() //设置为当前的协程调度器
    {
        t_scheduler=this;
    }

    void Scheduler::run()     //协程调度函数,创建idle、子协程，并执行子协程函数
    {
        SYLAR_LOG_DEBUG(g_logger) << m_name << " run";

        set_hook_enable(true);
        setThis();  //设置为当前的协程调度器
        if(sylar::GetThreadId()!=m_rootThread)  //当前线程不是主线程
        {
            t_fiber=Fiber::GetThis().get(); //通过智能指针获取到裸指针(Fiber::GetThis()是主线程的Fiber)
        }

        Fiber::ptr idle_fiber(new Fiber(std::bind(&Scheduler::idle,this))); //创建idle执行函数的Fiber
        Fiber::ptr cb_fiber;    //执行函数的协程

        FiberAndThread ft;
        while (true)
        {
            ft.reset();
            bool tickle_me=false;   //判断是否要通知协程调度器有任务
            bool is_active=false;
            {   //*取出我们要想执行的线程对应协程队列中的协程
                MutexType::Lock lock(m_mutex);
                auto it=m_fibers.begin();
                while (it!=m_fibers.end())
                {
                    if(it->thread!=-1 && it->thread!=sylar::GetThreadId())  //不需要执行的待执行协程(不是他想要执行的线程ID)
                    {
                        ++it;
                        tickle_me=true; //通知其他协程调度器有任务
                        continue;
                    }

                    SYLAR_ASSERT(it->fiber || it->cb);  
                    if(it->fiber && it->fiber->getState()==Fiber::EXEC)     //不需要执行的待执行协程(正在执行)
                    {
                        ++it;
                        continue;
                    }

                    ft=*it; //取出需要执行的待执行协程
                    m_fibers.erase(it); //删除list元素
                    ++m_activeThreadCount;
                    is_active=true;
                    break;
                }
            }
            if(tickle_me)   //通知协程调度器有任务
            {
                tickle();
            }

            if(ft.fiber && (ft.fiber->getState()!=Fiber::TERM   //已创建协程
                            && ft.fiber->getState()!= Fiber::EXCEPT))   //需要执行的协程不在结束、异常状态(需要执行)
            {
                ft.fiber->swapIn();
                --m_activeThreadCount;

                if(ft.fiber->getState()==Fiber::READY)  //协程处于可执行状态(未执行完的)
                {
                    schedule(ft.fiber); //重新调度协程(协程队列插入)
                }
                else if(ft.fiber->getState()!=Fiber::TERM && ft.fiber->getState()!=Fiber::EXCEPT)//需要执行的协程不在结束、异常状态(swapIn()有没有使处于执行中状态)
                {
                    ft.fiber->m_state=Fiber::HOLD;  //暂停状态
                }
                ft.reset();
            }
            else if(ft.cb)  //有执行函数，但是未创建协程    (未执行完的)
            {
                if(cb_fiber)
                {
                    cb_fiber->reset(ft.cb);
                }
                else
                {
                    cb_fiber.reset(new Fiber(ft.cb));
                }
                ft.reset();

                cb_fiber->swapIn();
                --m_activeThreadCount;

                if(cb_fiber->getState()==Fiber::READY)  //协程处于可执行状态
                {
                    schedule(cb_fiber); //调度协程
                    cb_fiber.reset();
                }
                else if(cb_fiber->getState()==Fiber::TERM || cb_fiber->getState()==Fiber::EXCEPT)   //需要执行的协程在结束、异常状态(swapIn()有没有使处于执行中状态)
                {
                    cb_fiber->reset(nullptr);
                }
                else    //其他状态
                {
                    cb_fiber->m_state=Fiber::HOLD;
                    cb_fiber.reset();
                }
            }
            else    //没有任务执行时执行idle协程(线程处于空闲状态)
            {
                if(is_active)
                {
                    --m_activeThreadCount;
                    continue;
                }
                if(idle_fiber->getState()==Fiber::TERM) //所有任务已完成
                {
                    SYLAR_LOG_INFO(g_logger)<<"idle fiber term";
                    break;
                }

                ++m_idleThreadCount;
                idle_fiber->swapIn();
                --m_idleThreadCount;
                if(idle_fiber->getState()!=Fiber::TERM && idle_fiber->getState()!=Fiber::EXCEPT)
                {
                    idle_fiber->m_state=Fiber::HOLD;
                }
            }
        }
    }
    //1.设置当前线程的scheduler
    //2.设置当前线程的run，fiber
    //3.协程调度循环while(true)
    //  1.协程消息队列里面是否有任务(有任务的执行任务)
    //  2.无任务执行，执行idle

    void Scheduler::switchTo(int thread)
    {
        SYLAR_ASSERT(Scheduler::GetThis() != nullptr);
        if(Scheduler::GetThis() == this)
        {
            if(thread == -1 || thread == sylar::GetThreadId())
            {
                return;
            }
        }
        schedule(Fiber::GetThis(), thread);
        Fiber::YieldToHold();   //将当前协程切换到后台，并将主协程切换出来
    }

    std::ostream& Scheduler::dump(std::ostream& os) 
    {
        os << "[Scheduler name=" << m_name
        << " size=" << m_threadCount
        << " active_count=" << m_activeThreadCount
        << " idle_count=" << m_idleThreadCount
        << " stopping=" << m_stopping
        << " ]" << std::endl << "    ";
        for(size_t i = 0; i < m_threadIds.size(); ++i)
        {
            if(i)
            {
                os << ", ";
            }
            os << m_threadIds[i];
        }
        return os;
    }

    void Scheduler::tickle()  //通知协程调度器有任务
    {
        SYLAR_LOG_INFO(g_logger)<<"tickle";
    }

    bool Scheduler::stopping()    //返回是否已经停止
    {
        MutexType::Lock lock(m_mutex);
        return m_autoStop && m_stopping
            && m_fibers.empty() && m_activeThreadCount==0;
    }

    void Scheduler::idle()    //协程无任务可调度、线程不能停止时执行idle协程(所有任务已完成)
    {
        SYLAR_LOG_INFO(g_logger)<<"idle";
        while (!stopping())
        {
            sylar::Fiber::YieldToHold();
        }
        
    }

    SchedulerSwitcher::SchedulerSwitcher(Scheduler* target)
    {
        m_caller = Scheduler::GetThis();
        if(target)
        {
            target->switchTo();
        }
    }

    SchedulerSwitcher::~SchedulerSwitcher() 
    {
        if(m_caller)
        {
            m_caller->switchTo();
        }
    }

}
