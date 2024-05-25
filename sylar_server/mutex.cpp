#include"mutex.h"
#include"macro.h"
#include"scheduler.h"
#include<stdexcept>

namespace sylar
{
    Semaphore::Semaphore(uint32_t count)    //创建信号量
    {
        if(sem_init(&m_semaphore,0,count))      //sem_init创建信号量
                                                //  ①信号量
                                                //  ②取0用于线程间；取非0（一般为1）用于进程间
                                                //  ③指定信号量初值
        {
            throw std::logic_error("sem_int error");
        }
    }
    Semaphore::~Semaphore() //销毁一个信号量
    {
        sem_destroy(&m_semaphore);  //销毁一个信号量
    }
    void Semaphore::wait()  //获取信号量(加锁等待)
    {
        if(sem_wait(&m_semaphore))  //给信号量加锁
        {
            throw std::logic_error("sem_wait error");
        }
        
    }
    void Semaphore::notify()    //释放信号量(解锁唤醒)
    {
        if(sem_post(&m_semaphore))  //给信号量解锁
        {
            throw std::logic_error("sem_post error");
        }
    }

    FiberSemaphore::FiberSemaphore(size_t initial_concurrency)
    :m_concurrency(initial_concurrency)
    {}

    FiberSemaphore::~FiberSemaphore()
    {
        SYLAR_ASSERT(m_waiters.empty());
    }

    bool FiberSemaphore::tryWait()
    {
        SYLAR_ASSERT(Scheduler::GetThis());
        {
            MutexType::Lock lock(m_mutex);
            if(m_concurrency > 0u)  //0u   无符号整型 0
            {
                --m_concurrency;
                return true;
            }
            return false;
        }
    }

    void FiberSemaphore::wait()
    {
        SYLAR_ASSERT(Scheduler::GetThis());
        {
            MutexType::Lock lock(m_mutex);
            if(m_concurrency > 0u)
            {
                --m_concurrency;
                return;
            }
            m_waiters.push_back(std::make_pair(Scheduler::GetThis(), Fiber::GetThis()));
        }
        Fiber::YieldToHold();   //将当前协程切换到后台，并将主协程切换出来
    }

    void FiberSemaphore::notify()
    {
        MutexType::Lock lock(m_mutex);
        if(!m_waiters.empty())
        {
            auto next = m_waiters.front();
            m_waiters.pop_front();
            next.first->schedule(next.second);
        }
        else
        {
            ++m_concurrency;
        }
    }    
}