#include"timer.h"
#include"util.h"

namespace sylar
{

    bool Timer::Comparator::operator() (const Timer::ptr& lhs,const Timer::ptr& rhs) const  //比较定时器的智能指针的大小(自定义升降序————这里按执行时间排序)
    {
        if(!lhs && !rhs)    //两个都不存在
        {
            return false;
        }
        if(!lhs)            //左边不存在，升序
        {
            return true;
        }
        if(!rhs)            //右边不存在
        {
            return false;
        }
        if(lhs->m_next<rhs->m_next) //比较时间，升序
        {
            return true;
        }
        if(rhs->m_next<lhs->m_next) //比较时间
        {
            return false;
        }
        return lhs.get() < rhs.get();   //时间相同，比较地址
    }
    
    Timer::Timer(uint64_t ms,std::function<void()> cb,bool recurring,TimerManager* manager) //初始化
        :m_recurring(recurring)
        ,m_ms(ms)
        ,m_cb(cb)
        ,m_manager(manager)
    {
        m_next=sylar::GetCurrentMS()+m_ms;
    }

    Timer::Timer(uint64_t next):m_next(next)    //用来判断是否超时的
    {}

    bool Timer::cancel()  //取消定时器
    {
        TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
        if(m_cb)    //
        {
            m_cb=nullptr;
            auto it=m_manager->m_timers.find(shared_from_this());   //shared_from_this()获取当前类的智能指针    find找到
            m_manager->m_timers.erase(it);  //删除对应的定时器管理的容器元素
            return true;
        }
        return false;
    }

    bool Timer::refresh() //刷新设置定时器的执行时间
    {
        TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
        if(!m_cb)
        {
            return false;
        }
        
        auto it=m_manager->m_timers.find(shared_from_this());   //shared_from_this()获取当前类的智能指针    find拷贝
        if (it==m_manager->m_timers.end())  //找不到
        {
            return false;
        }
        m_manager->m_timers.erase(it);  //删除
        m_next=sylar::GetCurrentMS()+m_ms;  //刷新时间
        m_manager->m_timers.insert(shared_from_this()); //重新插入
        return true;
    }

    bool Timer::reset(uint64_t ms,bool from_now)    //重置定时器执行周期,执行时间
    {
        if(ms==m_ms && !from_now)   //没有差别
        {
            return true;
        }
        TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
        if(!m_cb)
        {
            return false;
        }
        
        auto it=m_manager->m_timers.find(shared_from_this());   //shared_from_this()获取当前类的智能指针    find拷贝
        if (it==m_manager->m_timers.end())  //找不到
        {
            return false;
        }
        m_manager->m_timers.erase(it);  //删除
        uint64_t start=0;   //存定时器定义时间
        if(from_now)
        {
            start=sylar::GetCurrentMS();
        }
        else
        {
            start=m_next-m_ms;  //原本时间
        }
        m_ms=ms;
        m_next=start+m_ms;
        m_manager->addTimer(shared_from_this(),lock);
        return true;
    }

    TimerManager::TimerManager()
    {
        m_previouseTime=sylar::GetCurrentMS();
    }

    TimerManager::~TimerManager()
    {

    }

    Timer::ptr TimerManager::addTimer(uint64_t ms,std::function<void()> cb,bool recurring)  //添加定时器,并返回新插入的定时器
    {
        Timer::ptr timer(new Timer(ms,cb,recurring,this));
        RWMutexType::WriteLock lock(m_mutex);
        addTimer(timer,lock);
        return timer;
    }

    static void OnTimer(std::weak_ptr<void> weak_cond,std::function<void()> cb) //条件:如果对应的共享指针存在，执行定时器回调函数
    {
        std::shared_ptr<void> tmp=weak_cond.lock(); //返回weak_cond对应的存储空共享指针
        if(tmp)
        {
            cb();
        }
    }

    Timer::ptr TimerManager::addConditionTimer(uint64_t ms,std::function<void()> cb //添加条件定时器,并返回条件定时器
                            ,std::weak_ptr<void> weak_cond      //借助weak_ptr类型指针可以获取shared_ptr指针的一些状态信息(比如引用计数)
                            ,bool recurring)
    {
        return addTimer(ms,std::bind(&OnTimer,weak_cond,cb),recurring);
    }

    uint64_t TimerManager::getNextTimer()   //返回 现在到最近一个定时器执行的时间间隔(毫秒)
    {
        RWMutexType::ReadLock lock(m_mutex);
        m_tickled=false;
        if(m_timers.empty())
        {
            return ~0ull;   //返回最大数值
        }

        const Timer::ptr& next=*m_timers.begin();
        uint64_t now_ms=sylar::GetCurrentMS();
        if(now_ms>=next->m_next)    //该定时器执行已晚，需要执行
        {
            return 0;
        }
        else
        {
            return next->m_next-now_ms;
        }
    }

    void TimerManager::listExpiredCb(std::vector<std::function<void()>>& cbs) //获取需要执行(超时)的定时器对应的回调函数  列表
    {
        uint64_t now_ms=sylar::GetCurrentMS();
        std::vector<Timer::ptr> expired;    //存放已经超时的定时器,并释放

        {
            RWMutexType::ReadLock lock(m_mutex);
            if(m_timers.empty())
            {
                return;
            }
        }

        RWMutexType::WriteLock lock(m_mutex);

        bool rollover=detectClockRollover(now_ms);  //检测服务器时间是否被调后了(超过一个小时————更改之后比之前)
        if (!rollover && ((*m_timers.begin())->m_next > now_ms))  //没有超时
        {
            return;
        }
        
        Timer::ptr now_timer(new Timer(now_ms));
        auto it=rollover ? m_timers.end() : m_timers.lower_bound(now_timer);    //lower_bound()找到第一个大于等于now_timer的元素位置
        while (it!=m_timers.end() && (*it)->m_next==now_ms)
        {
            ++it;
        }
        expired.insert(expired.begin(),m_timers.begin(),it);    //插入超时的定时器
        m_timers.erase(m_timers.begin(),it);    //删除超时的
        cbs.reserve(expired.size());    //更改vector的容量

        for (auto& timer : expired) //把超时的定时器放在容器里
        {
            cbs.push_back(timer->m_cb);
            if (timer->m_recurring) //超时循环
            {
                timer->m_next=now_ms+timer->m_ms;
                m_timers.insert(timer);
            }
            else
            {
                timer->m_cb=nullptr;
            }
            
        }
        
    }

    bool TimerManager::hasTimer()    //判断定时管理器是否没有定时器
    {
        RWMutexType::ReadLock lock(m_mutex);
        return !m_timers.empty();
    }

    void TimerManager::addTimer(Timer::ptr val,RWMutexType::WriteLock& lock)  //将新定时器添加到管理器中
    {
        auto it=m_timers.insert(val).first;   //插入
        bool at_front=(it==m_timers.begin()) && !m_tickled;   //判断新的定时器是否插入到定时器的首部
        if (at_front)
        {
            m_tickled=true;
        }
        
        if(at_front)
        {
            onTimerInsetedAtFront();
        }
    }
    
    bool TimerManager::detectClockRollover(uint64_t now_ms)  //检测服务器时间是否被调后了(超过一个小时————更改之后比之前)
    {
        bool rollover=false;
        if(now_ms<m_previouseTime && now_ms<(m_previouseTime-60*60*1000))   //当更改之后比之前调后一个小时
        {
            rollover=true;
        }
        m_previouseTime=now_ms;
        return rollover;
    }

}