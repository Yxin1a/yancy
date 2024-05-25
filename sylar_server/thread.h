//线程相关的封装
//function————函数指针
//std::function<void()> cb————cb指向void*类型的函数，cb()来调用指向的函数

#ifndef __SYLAR_THREAD_H__
#define __SYLAR_THREAD_H__

#include"mutex.h"

//pthread_xxx
//std::thread C++11新特性,pthread C++98只对linux平台

namespace sylar
{

    /**
     *  线程类
     */
    class Thread : Noncopyable
    {
    public:
        typedef std::shared_ptr<Thread> ptr;    // 线程智能指针类型

        /**
         *  构造函数
         *       cb 线程执行函数
         *       name 线程名称
         */
        Thread(std::function<void()> cb,const std::string& name); 

        ~Thread();  //析构函数

        pid_t getId() const { return m_id; }    //获取线程id
        const std::string& getName() const { return m_name; }   //获取线程名称

        void join();    //等待线程执行完成

        static Thread* GetThis();   //获取当前的线程指针
        static const std::string& GetName();    //获取当前的线程名称

        /**
         *  设置当前线程名称
         *       name 线程名称
         */
        static void SetName(const std::string& name); 
        
    private:
        /*
            myClass()=delete;//表示删除默认构造函数
            myClass()=default;//表示默认存在构造函数
        */
        Thread(const Thread&)=delete;   //构造函数=delete————//表示删除默认拷贝构造函数，即不能进行默认拷贝
        Thread(const Thread&&)=delete;  //构造函数=delete————//表示删除默认拷贝构造函数，即不能进行默认拷贝
        Thread& operator=(const Thread&)=delete;    //构造函数=delete————//表示删除默认拷贝构造函数，即不能进行默认拷贝
    
        static void *run(void* arg);    //线程执行函数
    private:
        pid_t m_id=-1;              //线程id
        pthread_t m_thread=0;       //线程结构(线程指针)
        std::function<void()> m_cb; //线程执行函数(线程函数)
        std::string m_name;         //线程名称
        Semaphore m_semphore;       //信号量
    };
    
}

#endif