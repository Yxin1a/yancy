//  hook函数封装(将系统函数修改自己想要的函数)
//某些特殊时刻，系统内部预先设置好的函数，当系统周期到达指定时刻 会自动执行该'钩子'
//hook_sleep能实现和sleep的效果
//hook_sleep延迟时间是不会占用线程的时间，效率大大加大
#ifndef __SYLAR_HOOK_H__
#define __SYLAR_HOOK_H__

//socket相关(socket、connect客户连接服务、accept服务接受客户、close)
//io相关(read、readv、recv、recvfrom、recvmsg、
//      write、writev、send、sendto、sendmsg)
//fd相关(fcntl、ioctl、getsockopt获取一个套接字的选项、setsockopt设置套接字描述符的属性)
#include<unistd.h>
#include<time.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<sys/ioctl.h>
#include<fcntl.h>
#include<stdint.h>

namespace sylar
{
    bool is_hook_enable();  //返回当前线程是否hook
    void set_hook_enable(bool flag);    //设置当前线程的hook状态
}

extern "C"  //extern说明 此变量/函数是在别处定义的，要在此处引用
{
    //sleep
    //①修改后
    typedef unsigned int(*sleep_fun)(unsigned int seconds); //延时seconds秒级。   //typedef定义函数指针类型
    extern sleep_fun sleep_f;   //extern外部变量(全局静态存储区)————声明一个成员变量

    typedef int (*usleep_fun)(useconds_t usec); //延时usec微秒级  //typedef定义函数指针类型
    extern usleep_fun usleep_f; //extern外部变量(全局静态存储区)————声明一个成员变量

    typedef int (*nanosleep_fun)(const struct timespec *req,struct timespec *rem);  //延时seconds毫秒级。   //typedef定义函数指针类型
    extern nanosleep_fun nanosleep_f; //extern外部变量(全局静态存储区)————声明一个成员变量

    //②原函数
    // unsigned int sleep(unsigned int seconds);
    // int usleep(useconds_t usec);
    //int nanosleep(const struct timespec *req,struct timespec *rem);

    //socket
    //①修改后
    typedef int (*socket_fun)(int domain,int type,int protocol);    //typedef定义函数指针类型
    extern socket_fun socket_f; //extern外部变量(全局静态存储区)————声明一个成员变量

    extern int connect_with_timeout(int fd, const struct sockaddr* addr, socklen_t addrlen, uint64_t timeout_ms);   //(增加了超时变量)

    typedef int (*connect_fun)(int sockfd,const struct sockaddr* addr,socklen_t addrlen);   //客户端连接服务器地址
    extern connect_fun connect_f;   //extern外部变量(全局静态存储区)————声明一个成员变量
    
    typedef int (*accept_fun)(int s,struct sockaddr* addr,socklen_t* addrlen);  //服务器接收连接
    extern accept_fun accept_f; //extern外部变量(全局静态存储区)————声明一个成员变量

    //②原函数
    //int socket(int domain,int type,int protocol);
    //int connect(int sockfd,const struct sockaddr* addr,socklen_t addrlen);
    //int accept(int s,struct sockaddr* addr,socklen_t* addrlen);

    //read
    //①修改后
    typedef ssize_t (*read_fun)(int fd, void *buf, size_t count);
    extern read_fun read_f;
    
    typedef ssize_t (*readv_fun)(int fd, const struct iovec *iov, int iovcnt);
    extern readv_fun readv_f;
    
    typedef ssize_t (*recv_fun)(int sockfd, void *buf, size_t len, int flags);
    extern recv_fun recv_f;
    
    typedef ssize_t (*recvfrom_fun)(int sockfd, void *buf, size_t len, int flags,struct sockaddr *src_addr, socklen_t *addrlen);
    extern recvfrom_fun recvfrom_f;
    
    typedef ssize_t (*recvmsg_fun)(int sockfd, struct msghdr *msg, int flags);
    extern recvmsg_fun recvmsg_f;
    
    //②原函数
    //ssize_t read(int fd, void *buf, size_t count);
    //ssize_t readv(int fd, const struct iovec *iov, int iovcnt);
    //ssize_t recv(int sockfd, void *buf, size_t len, int flags);
    //ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,struct sockaddr *src_addr, socklen_t *addrlen);
    //ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags);

    //write
    //①修改后
    typedef ssize_t (*write_fun)(int fd, const void *buf, size_t count);
    extern write_fun write_f;

    typedef ssize_t (*writev_fun)(int fd, const struct iovec *iov, int iovcnt);
    extern writev_fun writev_f;
    
    typedef ssize_t (*send_fun)(int sockfd, const void *buf, size_t len, int flags);
    extern send_fun send_f;
    
    typedef ssize_t (*sendto_fun)(int sockfd, const void *buf, size_t len, int flags,const struct sockaddr *dest_addr, socklen_t addrlen);
    extern sendto_fun sendto_f;
    
    typedef ssize_t (*sendmsg_fun)(int sockfd, const struct msghdr *msg, int flags);
    extern sendmsg_fun sendmsg_f;
    
    //②原函数
    //ssize_t write(int fd, const void *buf, size_t count);
    //ssize_t writev(int fd, const struct iovec *iov, int iovcnt);
    //ssize_t send(int sockfd, const void *buf, size_t len, int flags);
    //ssize_t sendto(int sockfd, const void *buf, size_t len, int flags,const struct sockaddr *dest_addr, socklen_t addrlen);
    //ssize_t sendmsg(int sockfd, const struct msghdr *msg, int flags);

    //close
    //①修改后
    typedef int (*close_fun)(int fd);
    extern close_fun close_f;

    //②原函数
    //int close(int fd);
    
    //socket操作相关的
    //①修改后
    typedef int (*fcntl_fun)(int fd, int cmd,.../* arg */ );
    extern fcntl_fun fcntl_f;

    typedef int (*ioctl_fun)(int d, unsigned long int request, ...);
    extern ioctl_fun ioctl_f;

    typedef int (*getsockopt_fun)(int sockfd, int level, int optname,void *optval, socklen_t *optlen);
    extern getsockopt_fun getsockopt_f;

    typedef int (*setsockopt_fun)(int sockfd, int level, int optname,const void *optval, socklen_t optlen);
    extern setsockopt_fun setsockopt_f;

    //②原函数
    //int fcntl(int fd, int cmd, ... /* arg */ );
    //int ioctl(int d, unsigned long int request, ...);
    //int getsockopt(int sockfd, int level, int optname,void *optval, socklen_t *optlen);
    //int setsockopt(int sockfd, int level, int optname,const void *optval, socklen_t optlen);
}


#endif