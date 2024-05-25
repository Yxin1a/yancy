#include"fd_manager.h"
#include"hook.h"

#include<sys/types.h>
#include<sys/stat.h>
#include<unistd.h>

namespace sylar
{

    FdCtx::FdCtx(int fd)    //初始化，判断是否是socket，并设为非阻塞
        :m_isInit(false)
        ,m_isSocket(false)
        ,m_sysNonblock(false)
        ,m_userNonblock(false)
        ,m_isClosed(false)
        ,m_fd(fd)
        ,m_recvTimeout(-1)
        ,m_sendTimeout(-1)
    {
        init();
    }

    FdCtx::~FdCtx()
    {

    }

    bool FdCtx::init()  //初始化文件句柄，判断是否是socket，并设为非阻塞
    {
        if(m_isInit)
        {
            return true;
        }
        m_recvTimeout=-1;
        m_sendTimeout=-1;

        struct stat fd_stat;    //看文件是不是句柄
        if (-1==fstat(m_fd,&fd_stat))   //失败  fstat返回已打开的文件信息
        {
            m_isInit=false;
            m_isSocket=false;
        }
        else    //成功
        {
            m_isInit=true;
            m_isSocket=S_ISSOCK(fd_stat.st_mode);   //判断是否是socket
        }

        if(m_isSocket)
        {
            int flags=fcntl_f(m_fd,F_GETFL,0);  //经常用这个fcntl函数对已打开的文件描述符改变为非阻塞，返回文件描述符状态(是否阻塞)
            if (!(flags & O_NONBLOCK))  //不是非阻塞
            {
                fcntl_f(m_fd,F_SETFL,flags | O_NONBLOCK);   //加上非阻塞
            }
            m_sysNonblock=true;
        }
        else
        {
            m_sysNonblock=false;
        }

        m_userNonblock=false;
        m_isClosed=false;
        return m_isInit;        
    }

    void FdCtx::setTimeout(int type, uint64_t v)   //设置超时时间
    {
        if(type==SO_RCVTIMEO)  //SO_RCVTIMEO(读超时), SO_SNDTIMEO(写超时)
        {
            m_recvTimeout=v;
        }
        else
        {
            m_sendTimeout=v;
        }
        
    }

    uint64_t FdCtx::getTimeout(int type)   //获取超时时间
    {
        if (type==SO_RCVTIMEO)
        {
            return m_recvTimeout;
        }
        else
        {
            return m_sendTimeout;
        }
        
    }

    FdManager::FdManager()  //初始化 定义文件句柄集合大小
    {
        m_datas.resize(64);
    }

    FdCtx::ptr FdManager::get(int fd,bool auto_create)   //获取/创建、获取文件句柄类FdCtx
    {
        RWMutexType::ReadLock lock(m_mutex);
        if((int)m_datas.size()<=fd)  //不存在
        {
            if(auto_create==false)  //不自动创建
            {
                return nullptr;
            }
        }
        else    //存在
        {
            if(m_datas[fd] || !auto_create)
            {
                return m_datas[fd];
            }
        }
        lock.unlock();

        //自动创建，并插入容器
        RWMutexType::WriteLock lock2(m_mutex);
        FdCtx::ptr ctx(new FdCtx(fd));
        if(fd>=(int)m_datas.size()) //容器大小不够
        {
            m_datas.resize(fd*1.5);
        }
        m_datas[fd]=ctx;
        return ctx;
    }

    void FdManager::del(int fd)    //删除文件句柄类
    {
        RWMutexType::WriteLock lock(m_mutex);
        if((int)m_datas.size()<=fd)  //不存在
        {
            return;
        }
        m_datas[fd].reset();
    }
    
}