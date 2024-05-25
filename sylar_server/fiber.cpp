#include"fiber.h"
#include<malloc.h>
#include<atomic>

#include"config.h"
#include"macro.h"
#include"log.h"
#include"scheduler.h"

namespace sylar
{
    static Logger::ptr g_logger=SYLAR_LOG_NAME("system");

    //std::atomic原子操作：更小的代码片段，并且该片段必定是连续执行的，不可分割。
    //                    保护一个变量。
    static std::atomic<uint64_t> s_fiber_id {0};    //当前协程id
    static std::atomic<uint64_t> s_fiber_count {0}; //当前协程的总数量

    //thread_local————t_fiber仅可在thread_local上创建的协程上访问。用于协程本地对象的声明
    static thread_local Fiber* t_fiber=nullptr;     //当前的协程指针
    static thread_local Fiber::ptr t_threadFiber=nullptr;  //主协程的智能指针(主协程对应的主协程为空)

    //协程栈大小
    static ConfigVar<uint32_t>::ptr g_fiber_stack_size =
        Config::Lookup<uint32_t> ("fiber.stack_size",128 * 1024,"fiber stack size");

    //协程运行栈的分配
    class MallocStackAllocator
    {
    public:
        static void* Alloc(size_t size) //创建协程运行的栈
        {
            return malloc(size);
        }

        static void Dealloc(void* vp,size_t size)   //释放协程运行的栈
        {
            return free(vp);
        }
    };

    using StackAllocator=MallocStackAllocator;  //起别名

    Fiber::Fiber() //初始化——并创建主协程
    {
        m_state=EXEC;
        SetThis(this);  //回到当前协程

        if(getcontext(&m_ctx))  //getcontext() 获取当前上下文，并将当前的上下文保存到m_ctx(协程库)中
        {
            SYLAR_ASSERT2(false,"getcontext");
        }

        ++s_fiber_count;

        SYLAR_LOG_DEBUG(g_logger)<<"Fiber::Fiber";
    }

    Fiber::Fiber(std::function<void()> cb,size_t stacksize,bool use_caller) //初始化——并创建子协程 和 初始化协程
        :m_id(++s_fiber_id)
        ,m_cb(cb)
    {
        ++s_fiber_count;
        m_stacksize=stacksize ? stacksize : g_fiber_stack_size->getValue();

        m_stack=StackAllocator::Alloc(m_stacksize);     //创建协程运行的栈
        if(getcontext(&m_ctx))  //getcontext() 获取当前上下文，并将当前的上下文保存到m_ctx(协程库)中
        {
            SYLAR_ASSERT2(false,"getcontext");
        }

        m_ctx.uc_link=nullptr;              //uc_link上下文
        m_ctx.uc_stack.ss_sp=m_stack;       //uc_stack该上下文中使用的栈，ss_sp对应的指针
        m_ctx.uc_stack.ss_size=m_stacksize; //uc_stack该上下文中使用的栈，ss_size对应的大小

        if(!use_caller) //不在MainFiber上调度
        {
            makecontext(&m_ctx,&Fiber::MainFunc,0); //makecontext() 创建一个新的上下文。
                                                //原理：修改通过getcontext()取得的上下文m_ctx(这意味着调用makecontext前必须先调用getcontext)。
                                                //      然后给该上下文指定一个栈空间m_ctx->uc.stack,设置后继的上下文m_ctx->uc_link.
        }
        else    //在MainFiber上调度
        {
            makecontext(&m_ctx,&Fiber::CallerMainFunc,0); //makecontext() 创建一个新的上下文。
        }

        SYLAR_LOG_DEBUG(g_logger)<<"Fiber::Fiber id="<<m_id;
    }

    Fiber::~Fiber()   //释放协程运行的栈——运行协程置为空
    {
        --s_fiber_count;
        if(m_stack)
        {
            SYLAR_ASSERT(m_state==TERM || m_state==EXCEPT || m_state==INIT);

            StackAllocator::Dealloc(m_stack,m_stacksize);   //释放协程运行的栈
        }
        else
        {
            SYLAR_ASSERT(!m_cb);
            SYLAR_ASSERT(m_state==EXEC);

            Fiber* cur=t_fiber;
            if(cur==this)
            {
                SetThis(nullptr);
            }
        }
        SYLAR_LOG_DEBUG(g_logger)<<"Fiber::~Fiber id="<<m_id;
    }

    void Fiber::reset(std::function<void()> cb)   //重置协程执行函数,并设置状态(协程结束后，栈空间还没有释放，利用该栈执行新的协程)
    {
        SYLAR_ASSERT(m_stack);
        SYLAR_ASSERT(m_state==TERM || m_state==EXCEPT || m_state==INIT);

        m_cb=cb;
        if(getcontext(&m_ctx))  //getcontext() 获取当前上下文，并将当前的上下文保存到m_ctx(协程库)中
        {
            SYLAR_ASSERT2(false,"getcontext");
        }

        m_ctx.uc_link=nullptr;              //uc_link上下文
        m_ctx.uc_stack.ss_sp=m_stack;       //uc_stack该上下文中使用的栈，ss_sp对应的指针
        m_ctx.uc_stack.ss_size=m_stacksize; //uc_stack该上下文中使用的栈，ss_size对应的大小

        makecontext(&m_ctx,&Fiber::MainFunc,0); //makecontext() 创建一个新的上下文。
                                                //原理：修改通过getcontext()取得的上下文m_ctx(这意味着调用makecontext前必须先调用getcontext)。
                                                //      然后给该上下文指定一个栈空间m_ctx->uc.stack,设置后继的上下文m_ctx->uc_link.

        m_state=INIT;
    }

    void Fiber::swapIn()  //将当前协程切换到运行状态,并将主协程切换到后台(执行的是协程调度器的调度协程)
    {
        SetThis(this);  //回到当前协程
        SYLAR_ASSERT(m_state!=EXEC);
        m_state=EXEC;

        if(swapcontext(&Scheduler::GetMainFiber()->m_ctx,&m_ctx))   //切换上下文  t_threadFiber->m_ctx——切换——>m_ctx
                                                        //原理：保存当前上下文到oucp结构体中，然后激活upc上下文。 
        {//主协程切换到子协程是切换到最近创建的子协程
            SYLAR_ASSERT2(false,"swapcontext");
        }
    }

    void Fiber::call()    //将当前线程切换到执行状态,并将主协程切换到后台(执行的是当前线程的主协程)
    {
        SetThis(this);
        m_state=EXEC;
        if(swapcontext(&t_threadFiber->m_ctx,&m_ctx))
        {//主协程切换到子协程是切换到最近创建的子协程
            SYLAR_ASSERT2(false,"swapcontext");
        }
    }

    void Fiber::swapOut()     //将当前协程切换到后台,并将主协程切换出来(执行的是协程调度器的调度协程)
    {
        SetThis(Scheduler::GetMainFiber());

        if(swapcontext(&m_ctx,&Scheduler::GetMainFiber()->m_ctx))   //切换上下文  m_ctx——切换——>t_threadFiber->m_ctx
                                                        //原理：保存当前上下文到oucp结构体中，然后激活upc上下文。 
        {//主协程切换到子协程是切换到最近创建的子协程
            SYLAR_ASSERT2(false,"swapcontext");
        }
    }

    void Fiber::back()    //将当前线程切换到后台,并将主协程切换出来(执行的是当前线程的主协程)
    {
        SetThis(t_threadFiber.get());
        if(swapcontext(&m_ctx,&t_threadFiber->m_ctx))
        {//主协程切换到子协程是切换到最近创建的子协程
            SYLAR_ASSERT2(false,"swapcontext");
        }
    }

    void Fiber::SetThis(Fiber* f)   //设置当前线程的运行协程
    {
        t_fiber=f;
    }

    Fiber::ptr Fiber::GetThis()    //返回当前所在的协程，没有则创建新的主协程(无执行函数)
    {
        if(t_fiber)
        {
            return t_fiber->shared_from_this(); //shared_from_this()功能为返回一个当前类的std::share_ptr(共享指针)
        }

        Fiber::ptr main_fiber(new Fiber);   //创建新的主协程,并回到新创建的主协程
        
        SYLAR_ASSERT(t_fiber==main_fiber.get());    //判断当前协程是否是主协程
        t_threadFiber=main_fiber;
        return t_fiber->shared_from_this();
    }

    void Fiber::YieldToReady() //将当前协程切换到后台，并将主协程切换出来,并设置为READY状态
    {
        Fiber::ptr cur=GetThis();
        cur->m_state=READY;
        cur->swapOut();
    }

    void Fiber::YieldToHold()  //将当前协程切换到后台，并将主协程切换出来,并设置为HOLD状态
    {
        Fiber::ptr cur=GetThis();
        cur->m_state=HOLD;
        cur->swapOut();
    }

    uint64_t Fiber::TotalFibers()  //返回当前协程的总数量
    {
        return s_fiber_count;
    }

    void Fiber::MainFunc()  //执行协程执行函数(执行完成返回到线程主协程)
    {
        Fiber::ptr cur=GetThis();   //共享指针+1
        SYLAR_ASSERT(cur);
        try     //可能出现异常的代码
        {
            cur->m_cb();    //执行协程中的函数
            cur->m_cb=nullptr;
            cur->m_state=TERM;  //协程执行完
        }
        catch(std::exception& ex)   //异常处理
        {
            cur->m_state=EXCEPT;
            SYLAR_LOG_ERROR(g_logger)<<"Fiber Except: "<<ex.what()
                <<" fiber_id="<<cur->getId()
                <<std::endl
                <<sylar::BacktraceToString();
        }
        catch(...)  //异常处理
        {
            cur->m_state=EXCEPT;
            SYLAR_LOG_ERROR(g_logger)<<"Fiber Except"
                <<" fiber_id="<<cur->getId()
                <<std::endl
                <<sylar::BacktraceToString();
        }
        
        auto raw_ptr=cur.get(); //通过智能指针获取到裸指针
        cur.reset();    //释放cur(共享指针-1)
        raw_ptr->swapOut(); //切换回主协程——释放主协程

        SYLAR_ASSERT2(false,"never reach fiber_id="+std::to_string(raw_ptr->getId()));
    }
    
    void Fiber::CallerMainFunc()    //执行协程执行函数(执行完成返回到线程调度协程)
    {
        Fiber::ptr cur=GetThis();   //共享指针+1
        SYLAR_ASSERT(cur);
        try     //可能出现异常的代码
        {
            cur->m_cb();    //执行协程中的函数
            cur->m_cb=nullptr;
            cur->m_state=TERM;  //协程执行完
        }
        catch(std::exception& ex)   //异常处理
        {
            cur->m_state=EXCEPT;
            SYLAR_LOG_ERROR(g_logger)<<"Fiber Except: "<<ex.what()
                <<" fiber_id="<<cur->getId()
                <<std::endl
                <<sylar::BacktraceToString();
        }
        catch(...)  //异常处理
        {
            cur->m_state=EXCEPT;
            SYLAR_LOG_ERROR(g_logger)<<"Fiber Except"
                <<" fiber_id="<<cur->getId()
                <<std::endl
                <<sylar::BacktraceToString();
        }
        
        auto raw_ptr=cur.get(); //通过智能指针获取到裸指针
        cur.reset();    //释放cur(共享指针-1)
        raw_ptr->back(); //切换回主协程——释放主协程

        SYLAR_ASSERT2(false,"never reach fiber_id="+std::to_string(raw_ptr->getId()));
    }

    uint64_t Fiber::GetFiberId()   //获取当前协程的id
    {
        if(t_fiber)
        {
            return t_fiber->getId();
        }
        return 0;
    }

}