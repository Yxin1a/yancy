//  定时器封装
#ifndef __SYLAR_TIMER_H__
#define __SYLAR_TIMER_H__

#include<memory>
#include<set>
#include<vector>
#include"thread.h"

namespace sylar
{

    class TimerManager;

    /**
     *  定时器
     */
    class Timer:public std::enable_shared_from_this<Timer>//enable_shared_from_this<>成员函数可以获取自身类的智能指针,Time只能以智能指针形式存在
    {
    friend class TimerManager;
    public:
        typedef std::shared_ptr<Timer> ptr; //定时器的智能指针类型

        /**
         *  构造函数(用来判断是否超时的)
         *      next 精确的执行时间(毫秒)
         */
        Timer(uint64_t next);
        
        bool cancel();  //取消定时器

        bool refresh(); //刷新设置定时器的执行时间

        /**
         *  重置定时器执行周期,执行时间
         *      ms 定时器执行间隔时间(毫秒)
         *      from_now 是否从当前时间开始计算
         */
        bool reset(uint64_t ms,bool from_now);

    private:

        /**
         *  构造函数
         *      ms 定时器执行间隔时间(执行周期)
         *      cb 回调函数
         *      recurring 是否循环
         *      manager 定时器管理器
         */
        Timer(uint64_t ms,std::function<void()> cd,bool recurring,TimerManager* manager);

    private:
        bool m_recurring=false; //是否循环定时器
        uint64_t m_ms=0;        //执行周期
        uint64_t m_next=0;      //精确的执行时间(循环下一次的时间)
        std::function<void()> m_cb; //回调函数
        TimerManager* m_manager=nullptr;    //定时器管理器

    private:

        /**
         *  定时器比较仿函数(自定义升降序)
         */
        struct Comparator
        {
            /**
             *  比较定时器的智能指针的大小(自定义升降序————这里按执行时间排序)
             *      lhs 定时器智能指针
             *      rhs 定时器智能指针
             */
            bool operator() (const Timer::ptr& lhs,const Timer::ptr& rhs) const;
        };
        
    };

    /**
     *  定时器管理器
     */
    class TimerManager
    {
    friend class Timer;
    public:
        typedef RWMutex RWMutexType;    //读写锁类型

        /**
         *  构造函数
         */
        TimerManager();

        /**
         *  析构函数
         */
        virtual ~TimerManager();

        /**
         *  添加定时器,并返回新插入的定时器
         *      ms 定时器执行间隔时间
         *      cb 定时器回调函数
         *      recurring 是否循环定时器
         */
        Timer::ptr addTimer(uint64_t ms,std::function<void()> cb,bool recurring=false);

        /**
         *  添加条件定时器,并返回条件定时器
         *      ms 定时器执行间隔时间
         *      cb 定时器回调函数
         *      weak_cond 条件
         *      recurring 是否循环定时器
         */
        Timer::ptr addConditionTimer(uint64_t ms,std::function<void()> cb
                                ,std::weak_ptr<void> weak_cond      //借助weak_ptr类型指针可以获取shared_ptr指针的一些状态信息(比如引用计数)
                                ,bool recurring=false);

        uint64_t getNextTimer();    //返回现在到最近一个定时器执行的时间间隔(毫秒)

        /**
         *  获取需要执行(超时)的定时器对应的回调函数  列表
         *      cbs 回调函数数组
         */
        void listExpiredCb(std::vector<std::function<void()>>& cbs);

        bool hasTimer();    //判断定时管理器是否没有定时器

    protected:

        virtual void onTimerInsetedAtFront()=0;   //当有新的定时器插入到定时器的首部,执行该函数

        void addTimer(Timer::ptr val,RWMutexType::WriteLock& lock);  //将新定时器添加到管理器中

    private:

        bool detectClockRollover(uint64_t now_ms);  //检测服务器时间是否被调后了(超过一个小时————更改之后比之前)

    private:
        RWMutexType m_mutex;    //互斥量
        std::set<Timer::ptr,Timer::Comparator> m_timers;    //定时器集合
        bool m_tickled=false;   //是否触发onTimerInsetedAtFront
        uint64_t m_previouseTime = 0;   //上次执行时间(之前————比较————更改之后)
    };

}
#endif