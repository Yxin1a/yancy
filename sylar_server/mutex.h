//  信号量和各种锁的封装
#ifndef __SYLAR_MUTEX_H__
#define __SYLAR_MUTEX_H__

#include<thread>
#include<functional>
#include<memory>
#include<iostream>
#include<pthread.h>
#include<semaphore.h>
#include<stdint.h>
#include<atomic>
#include<list>

#include"noncopyable.h"
#include"fiber.h"

namespace sylar
{

    /**
     *  信号量(整形信号量)
     *      ①(互斥量)(最好)
     *      ②(实现同步，使得并发、切换 能按照一定顺序运行)
     */
    class Semaphore : Noncopyable
    {
    public:
        /**
         *  构造函数，创建信号量
         *      count 信号量值的大小(表示系统中某种资源的数量)
         */
        Semaphore(uint32_t count=0);
        
        /**
         *  析构函数，销毁一个信号量
         */
        ~Semaphore();
        
        void wait();    //获取信号量(加锁等待)
        void notify();  //释放信号量(解锁唤醒)
    private:
        Semaphore(const Semaphore&)=delete;   //构造函数=delete————//表示删除默认拷贝构造函数，即不能进行默认拷贝
        Semaphore(const Semaphore&&)=delete;  //构造函数=delete————//表示删除默认拷贝构造函数，即不能进行默认拷贝
        Semaphore& operator=(const Semaphore&)=delete;    //构造函数=delete————//表示删除默认拷贝构造函数，即不能进行默认拷贝
    
    private:
        sem_t m_semaphore;  //信号量
    };

    /**
     *  局部锁的模板实现
     *      构造函数加锁
     *      析构函数解锁
     */
    template<class T>
    struct ScopedLockImpl
    {
    public:
        /**
         *  构造函数,加锁
         *      mutex Mutex
         */
        ScopedLockImpl(T& mutex):m_mutex(mutex)
        {
            m_mutex.lock();
            m_locked=true;
        }

        /**
         *  析构函数,自动释放锁
         */
        ~ScopedLockImpl()
        {
            unlock();
        }

        void lock()     //加锁
        {
            if(!m_locked)
            {
                m_mutex.lock();
                m_locked=true;
            }
        }
        void unlock()   //解锁
        {
            if(m_locked)
            {
                m_mutex.unlock();
                m_locked=false;
            }
        }
    private:
        T& m_mutex;     // mutex(锁)
        bool m_locked;  // 是否已上锁
    };

    /**
     *  局部读锁模板实现
     *      构造函数加锁
     *      析构函数解锁
     */
    template<class T>
    struct ReadScopedLockImpl
    {
    public:
        /**
         *  构造函数,加锁
         *      mutex 读写锁
         */
        ReadScopedLockImpl(T& mutex):m_mutex(mutex)
        {
            m_mutex.rdlock();   //rdlock()不是库函数，是RWMutex类的成员函数
            m_locked=true;
        }

        /**
         *  析构函数,自动释放锁
         */
        ~ReadScopedLockImpl()
        {
            unlock();   //unlock()不是库函数，是RWMutex类的成员函数
        }

        void lock()     //加读锁
        {
            if(!m_locked)
            {
                m_mutex.rdlock();   //rdlock()不是库函数，是RWMutex类的成员函数
                m_locked=true;
            }
        }
        void unlock()   //释放锁
        {
            if(m_locked)
            {
                m_mutex.unlock();   //unlock()不是库函数，是RWMutex类的成员函数
                m_locked=false;
            }
        }
    private:
        T& m_mutex;     // mutex(锁)
        bool m_locked;  // 是否已上锁
    };

    /**
     *  局部写锁模板实现
     *      构造函数加锁
     *      析构函数解锁
     */
    template<class T>
    struct WriteScopedLockImpl
    {
    public:
        /**
         *  构造函数,加锁
         *      mutex 读写锁
         */
        WriteScopedLockImpl(T& mutex):m_mutex(mutex)
        {
            m_mutex.wrlock();   //wrlock()不是库函数，是RWMutex类的成员函数
            m_locked=true;
        }

        /**
         *  析构函数,自动释放锁
         */
        ~WriteScopedLockImpl()
        {
            unlock();   //unlock()不是库函数，是RWMutex类的成员函数
        }

        void lock()     //加写锁
        {
            if(!m_locked)
            {
                m_mutex.wrlock();   //wrlock()不是库函数，是RWMutex类的成员函数
                m_locked=true;
            }
        }
        void unlock()   //释放锁
        {
            if(m_locked)
            {
                m_mutex.unlock();   //unlock()不是库函数，是RWMutex类的成员函数
                m_locked=false;
            }
        }
    private:
        T& m_mutex;     // mutex(锁)
        bool m_locked;  // 是否已上锁
    };

    /**
     *  互斥量
     */
    class Mutex : Noncopyable
    {
    public:
        typedef ScopedLockImpl<Mutex> Lock;     // 局部锁
        
        /**
         *  构造函数
         */
        Mutex()
        {
            pthread_mutex_init(&m_mutex,nullptr);   //初始化一个互斥锁(互斥量)
        }

        /**
         *  析构函数
         */
        ~Mutex()
        {
            pthread_mutex_destroy(&m_mutex);        //销毁一个互斥锁
        }

        void lock()     //加锁
        {
            pthread_mutex_lock(&m_mutex);           //加锁
        }
        void unlock()   //解锁
        {
            pthread_mutex_unlock(&m_mutex);         //解锁
        }
    private:
        pthread_mutex_t m_mutex;    //锁
    };

    /**
     *  空锁(用于调试)
     */
    class NullMutex : Noncopyable
    {
    public:
        typedef ScopedLockImpl<NullMutex> Lock;     //局部锁

        /**
         *  构造函数
         */
        NullMutex(){}

        /**
         *  析构函数
         */
        ~NullMutex(){}

        void lock(){}   //加锁
        void unlock(){} //解锁
    };

// 1、读写锁是“写模式加锁”时, 解锁前，所有对该锁加锁的线程都会被阻塞。
// 2、读写锁是“读模式加锁”时, 如果线程以读模式对其加锁会成功；如果线程以写模式加锁会阻塞。
// 3、读写锁是“读模式加锁”时, 既有试图以写模式加锁的线程，也有试图以读模式加锁的线程。
//                          那么读写锁会阻塞随后的读模式锁请求。优先满足写模式锁。读锁、写锁并行阻塞，写锁优先级高

    /**
     *  读写互斥量
     *      一般是——读多写少
     *      (能读锁就读锁，写锁包括读锁)
     */
    class RWMutex : Noncopyable
    {
    public:
        typedef ReadScopedLockImpl<RWMutex> ReadLock;   // 局部读锁
        typedef WriteScopedLockImpl<RWMutex> WriteLock; // 局部写锁
        
        /**
         *  构造函数
         */
        RWMutex()
        {
            pthread_rwlock_init(&m_lock,nullptr);   //pthread_rwlock_init()初始化一把读写锁
                                                    //  ①需要初始化的读写锁
                                                    //  ②NULL
        }

        /**
         *  析构函数
         */
        ~RWMutex()
        {
            pthread_rwlock_destroy(&m_lock);        //pthread_rwlock_destroy()销毁一把读写锁
        }

        void rdlock()   //上读锁
        {
            pthread_rwlock_rdlock(&m_lock); //pthread_rwlock_rdlock()以读方式请求读写锁
        }
        void wrlock()   //上写锁
        {
            pthread_rwlock_wrlock(&m_lock); //pthread_rwlock_wrlock()以写方式请求读写锁
        }
        void unlock()   //解锁
        {
            pthread_rwlock_unlock(&m_lock); //pthread_rwlock_unlock()解读写锁
        }
    private:
        pthread_rwlock_t m_lock;    // 读写锁
    };

    /**
     *  空读写锁(用于调试)
     */
    class NullRWMutex : Noncopyable
    {
    public:
        typedef ReadScopedLockImpl<NullMutex> ReadLock;     //局部读锁
        typedef WriteScopedLockImpl<NullMutex> WriteLock;   //局部写锁

        /**
         *  构造函数
         */
        NullRWMutex(){}

        /**
         *  析构函数
         */
        ~NullRWMutex(){}

        void rdlock(){} //加读锁
        void wrlock(){} //加写锁
        void unlock(){} //解锁
    };

    /**
     *  自旋锁
     *      (自旋锁的一个重要特点是它不会导致调用者睡眠，如果自旋锁已经被占用，调用者会一直处于忙等待状态，直到能够获取到锁。
     *       这就意味着自旋锁应当只在持锁时间短并且线程不会被阻塞的情况下使用，否则会浪费处理器时间，降低多处理器系统的并行性能。)
     */
    class Spinlock : Noncopyable
    {
    public:
    typedef ScopedLockImpl<Spinlock> Lock;  //局部锁
        /**
         *  构造函数
         */
        Spinlock()
        {
            pthread_spin_init(&m_mutex,0);      //pthread_spin_init()初始化一个自旋锁
        }

        /**
         *  析构函数
         */
        ~Spinlock()
        {
            pthread_spin_destroy(&m_mutex);     //pthread_spin_destroy()销毁一个自旋锁
        }

        void lock()     //上锁
        {
            pthread_spin_lock(&m_mutex);        //pthread_spin_lock()获取一个自旋锁
        }
        void unlock()   //解锁
        {
            pthread_spin_unlock(&m_mutex);      //pthread_spin_unlock()释放一个自旋锁
        }
    private:
        pthread_spinlock_t m_mutex;     //自旋锁
    };

    /**
     *  原子锁
     *      （更小的代码片段，并且该片段必定是连续执行的，不可分割。）
     */
    class CASLock : Noncopyable
    {
    public:
        typedef ScopedLockImpl<CASLock> Lock;   //局部锁
        
        /**
         *  构造函数
         */
        CASLock()
        {
            m_mutex.clear();    //刷新，清除之前的数据
        }

        /**
         *  析构函数
         */
        ~CASLock()
        {}

        void lock()     //上锁
        {
            while (std::atomic_flag_test_and_set_explicit(&m_mutex,std::memory_order_acq_rel))  //atomic_flag_test_and_set_explicit（）检测并设置 std::atomic_flag 的值，并返回 std::atomic_flag 的旧值
                                                                                                //  ①对象
                                                                                                //  ②内存序
            {}
        }
        void unlock()   //解锁
        {
            std::atomic_flag_clear_explicit(&m_mutex,std::memory_order_release);    //atomic_flag_clear_explicit（）清除 std::atomic_flag 对象，并设置它的值为 false
                                                                                    //  ①对象
                                                                                    //  ②内存序
        }
    private:
        volatile std::atomic_flag m_mutex;      //原子状态
        //volatile作用：每次取值的都是刷新之后的值（当一个被volatile关键字修饰的变量被一个线程修改的时候，其他线程可以立刻得到修改之后的结果）
    };
    
    /**
     * @brief 协程信号量
    */
    class Scheduler;
    class FiberSemaphore : Noncopyable
    {
    public:
        typedef Spinlock MutexType;

        FiberSemaphore(size_t initial_concurrency = 0);
        ~FiberSemaphore();

        /**
         * @brief 调并发的协程（加锁）
        */
        bool tryWait();
        /**
         * @brief 将当前协程调到等待队列中（加锁）
        */
        void wait();
        /**
         * @brief 将等待队列中调到执行队列中，并执行协程（解锁唤醒）
         *      优先调并发的协程
        */
        void notify();

        /**
         * @brief 获取并发的协程条数
        */
        size_t getConcurrency() const { return m_concurrency;}
        /**
         * @brief 刷新并发的协程条数
        */
        void reset() { m_concurrency = 0;}
    private:
        MutexType m_mutex;
        /// 正在等待的 协程——对应的协程调度器

        std::list<std::pair<Scheduler*,Fiber::ptr>> m_waiters;
        /// 并发的协程条数
        size_t m_concurrency;
    };

}

#endif