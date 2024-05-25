//协程封装————一个类对应一个协程
/*
    主协程————>子协程 (嵌套协程)
    ①在主协程创建子协程，不在子协程创建 子子协程
    ②子协程结束之后，切换回主协程

    创建协程(无执行函数、有执行函数)
    //协程封装————协程切换是切换到最近创建的协程
*/
#ifndef __SYLAR_FIBER_H__
#define __SYLAR_FIBER_H__

#include<memory>
#include<functional>
#include<ucontext.h>    //协程库

class Scheduler;
namespace sylar
{

    /**
     *  协程类
     */
    class Fiber:public std::enable_shared_from_this<Fiber>//enable_shared_from_this<>成员函数可以获取自身类的智能指针,Time只能以智能指针形式存在
    {
    friend class Scheduler;
    public:
        typedef std::shared_ptr<Fiber> ptr;

        /**
         *  协程状态
         */
        enum State
        {
            INIT,   //初始化状态(刚开始)
            EXEC,   //执行中状态(创建时)
            READY,  //可执行状态(切换后)
            HOLD,   //暂停状态(切换后)
            EXCEPT,  //异常状态(异常时)
            TERM   //结束状态(结束时)
        };

    private:
        /**
         *  无参构造函数
         *      每个线程第一个协程的构造
         */
        Fiber();

    public:
        /**
         *  构造函数
         *      cb 协程执行的函数
         *      stacksize 协程栈大小
         *      use_caller 是否在MainFiber上调度
         */
        Fiber(std::function<void()> cb,size_t stacksize=0,bool use_caller = false);

        /**
         *  析构函数
         */
        ~Fiber();

        /**
         *  重置协程执行函数,并设置状态(协程结束后，栈空间还没有释放，利用该栈执行新的协程)
         *      getState() 为 INIT, TERM, EXCEPT
         *      getState() = INIT
         */
        void reset(std::function<void()> cb);

        /**
         *  将当前协程切换到运行状态，并将主协程切换到后台(执行的是协程调度器的调度协程)
         *      getState() != EXEC
         *      getState() = EXEC
         */
        void swapIn();
        
        /**
         *  将当前协程切换到运行状态，并将主协程切换到后台(执行的是当前线程的主协程)
         *      getState() = EXEC
         */
        void call();

        void swapOut();     //将当前协程切换到后台,并将主协程切换出来(执行的是协程调度器的调度协程)

        void back();    //将当前线程切换到后台,并将主协程切换出来(执行的是当前线程的主协程)
        
        uint64_t getId() const { return m_id; } //返回协程id

        State getState() const { return m_state; }  //返回协程状态
        
    public:
        /**
         *  设置当前线程的运行协程
         *      f 运行协程
         */
        static void SetThis(Fiber* f);
        
        static Fiber::ptr GetThis();    //返回当前所在的协程，没有则创建新的主协程(无执行函数)
        
        /**
         *  将当前协程切换到后台，并将主协程切换出来,并设置为READY状态
         *      getState() = READY
         */
        static void YieldToReady();

        /**
         *  将当前协程切换到后台，并将主协程切换出来,并设置为HOLD状态
         *      getState() = HOLD
         */
        static void YieldToHold();

        static uint64_t TotalFibers();  //返回当前协程的总数量

        /**
         *  协程执行函数
         *      执行完成返回到线程主协程
         */
        static void MainFunc();

        /**
         *  协程执行函数
         *      执行完成返回到线程调度协程
         */
        static void CallerMainFunc();

        static uint64_t GetFiberId();   //获取当前协程的id
        
    private:
        uint64_t m_id=0;        //协程id
        uint32_t m_stacksize=0; //协程运行栈大小
        State m_state=INIT;     //协程状态

        ucontext_t m_ctx;       //协程上下文
        void* m_stack=nullptr;  //协程运行栈指针

        std::function<void()> m_cb; //协程运行函数
    };
}

#endif