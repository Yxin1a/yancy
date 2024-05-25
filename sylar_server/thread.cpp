#include"thread.h"
#include"log.h"
#include"util.h"

namespace sylar
{
    //thread_local————t_thread仅可在thread_local上创建的线程上访问。用于线程本地对象的声明
    static thread_local Thread* t_thread=nullptr;       //当前的线程指针
    static thread_local std::string t_thread_name="UNKNOW";     //当前的线程名称

    static sylar::Logger::ptr g_logger=SYLAR_LOG_NAME("system");

    Thread* Thread::GetThis()   //获取当前的线程指针
    {
        return t_thread;
    }
    const std::string& Thread::GetName()    //获取当前的线程名称
    {
        return t_thread_name;
    }
    void Thread::SetName(const std::string& name)   //设置当前线程名称
    {
        if(name.empty())
        {
            return;
        }
        if(t_thread)    //判断当前的线程指针是否为空
        {
            t_thread_name=name;
        }
        t_thread_name=name;
    }

    Thread::Thread(std::function<void()> cb,const std::string& name):m_cb(cb),m_name(name)    //线程类初始化——创建线程
    {
        if(name.empty())
        {
            m_name="UNKNOW";
        }
        int rt=pthread_create(&m_thread,nullptr,&Thread::run,this);     //pthread_create创建线程————成功：0；失败：错误号
                                                                        //① m_thread指向的内存单元被设置为新创建线程的线程ID
                                                                        //② 用于定制各种不同的线程属性  通常直接设为NULL
                                                                        //③ 新创建线程从此函数开始运行  为执行函数
                                                                        //④ 执行函数的参数。
        if(rt)  //线程创建失败
        {
            SYLAR_LOG_ERROR(g_logger)<<"pthread_create thread fail,rt="<<rt<<" name="<<name;
            throw std::logic_error("pthread_create error"); //logic_error报告错误
        }
        m_semphore.wait();  //加锁等待——保证原子性(保证创建线程成功)
    }
    Thread::~Thread()   //线程分离
    {
        if(m_thread)
        {
            pthread_detach(m_thread);   //实现该线程与主线程分离————成功：0；失败：错误号
        }
    }

    void Thread::join()
    {
        if(m_thread)    //线程是否存在
        {
            int rt=pthread_join(m_thread,nullptr);  //pthread_join以阻塞的方式等待m_thread线程结束————成功：0；失败：错误号
                                                    //① 线程指针
                                                    //② 指针，用来存储被等待线程的返回值
            if(rt)
            {
                SYLAR_LOG_ERROR(g_logger)<<"pthread_join thread fail,rt="<<rt<<" name="<<m_name;
                throw std::logic_error("pthread_join error");   //logic_error报告错误
            }
            m_thread=0;
        }
    }

    void* Thread::run(void* arg)    //线程执行函数————void* 是一个指向未知类型的指针（可以转换成其他类型）
    {
        Thread* thread=(Thread*)arg;    //(Thread*)arg强制类型转换
        t_thread=thread;
        t_thread_name=thread->m_name;
        thread->m_id=sylar::GetThreadId();
        pthread_setname_np(pthread_self(),thread->m_name.substr(0,15).c_str()); //pthread_setname_np设置指定线程的名称————pthread_self用于获取调用线程的线程ID

        std::function<void()> cb;
        cb.swap(thread->m_cb);  //swap交换function指向的函数

        thread->m_semphore.notify();    //解锁唤醒

        cb();   //执行函数
        return 0;
    }

}