//  文件句柄管理类(判断是否是socket)
#ifndef __FD_MANAGER_H__
#define __FD_MANAGER_H__

#include<memory>
#include<vector>
#include"thread.h"
#include"singLeton.h"

namespace sylar
{

    /**
     *  件句柄上下文类
     *      管理文件句柄类型(是否socket)
     *      是否阻塞,是否关闭,读/写超时时间
     */
    class FdCtx:public std::enable_shared_from_this<FdCtx>  //enable_shared_from_this<>成员函数可以获取自身类的智能指针,FdCtx只能以智能指针形式存在
    {
    public:
        typedef std::shared_ptr<FdCtx> ptr;

        /**
         *  通过文件句柄构造FdCtx
         */
        FdCtx(int fd);

        /**
         *  析构函数
         */
        ~FdCtx();

        bool isInit() const { return m_isInit;} //获取是否初始化完成
        bool isSocket() const { return m_isSocket;} //获取是否socket
        bool isClose() const { return m_isClosed;}  //获取是否已关闭

        /**
         *  设置用户主动设置非阻塞(用户态)
         *      v 是否阻塞
         */
        void setUserNonblock(bool v) { m_userNonblock = v;}

        bool getUserNonblock() const { return m_userNonblock;}  //获取是否用户主动设置的非阻塞 

        /**
         *  设置系统非阻塞(系统态)
         *      v 是否阻塞
         */
        void setSysNonblock(bool v) { m_sysNonblock = v;}

        bool getSysNonblock() const { return m_sysNonblock;}    //获取系统非阻塞

        /**
         *  设置超时时间
         *      type 类型SO_RCVTIMEO(读超时), SO_SNDTIMEO(写超时)
         *      v 时间毫秒
         */
        void setTimeout(int type, uint64_t v);

        /**
         *  获取超时时间
         *      type 类型SO_RCVTIMEO(读超时), SO_SNDTIMEO(写超时)
         *      (超时时间毫秒)
         */
        uint64_t getTimeout(int type);

    private:

        bool init();    //初始化文件句柄，判断是否是socket，并设为非阻塞

    private:

        bool m_isInit : 1;  //是否初始化
        bool m_isSocket : 1;//是否socket
        bool m_sysNonblock : 1; //是否系统hook非阻塞(系统态)
        bool m_userNonblock: 1; //是否用户主动设置hook非阻塞(用户态)
        bool m_isClosed: 1; //是否关闭
        int m_fd;   //文件句柄
        uint64_t m_recvTimeout; //读超时时间毫秒
        uint64_t m_sendTimeout; //写超时时间毫秒
    };

    /**
     *  文件句柄管理类
     */
    class FdManager
    {
    public:
        typedef RWMutex RWMutexType;

        /**
         *  无参构造函数
         */
        FdManager();

        /**
         *  获取/创建文件句柄类FdCtx
         *      fd 文件句柄
         *      auto_create 是否自动创建
         *      (返回对应文件句柄类FdCtx::ptr)
         */
        FdCtx::ptr get(int fd,bool auto_create=false);

        /**
         *  删除文件句柄类
         *      fd 文件句柄
         */
        void del(int fd);

    private:

        RWMutexType m_mutex;    //读写锁
        std::vector<FdCtx::ptr> m_datas;    //文件句柄集合
    };

    typedef Singleton<FdManager> FdMgr; //单例模式封装

}

#endif