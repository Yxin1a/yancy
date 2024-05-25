/**
 * @file socket_stream.h
 * @brief Socket流式接口封装
 *      (针对业务级接口封装)
 *      针对文件/socket封装
 *      一个类对象代表一个客户端的连接,一条数据连接
 * @author sylar.yin
 * @email 2033743028@qq.com
 * @date 2024-4-25
*/

#ifndef __SYLAR_SOCKET_STREAM_H__
#define __SYLAR_SOCKET_STREAM_H__

#include "../sylar_server/stream.h"
#include "../sylar_server/socket.h"
#include "../sylar_server/mutex.h"
#include "../sylar_server/iomanager.h"

namespace sylar
{

    /**
     *  Socket流
     * (一个类对象代表一个客户端的连接)
     */
    class SocketStream : public Stream
    {
    public:
        typedef std::shared_ptr<SocketStream> ptr;

        /**
         *  构造函数
         *      sock Socket类
         *      owner 是否完全控制
         */
        SocketStream(Socket::ptr sock, bool owner = true);

        /**
         *  析构函数
         *      如果m_owner=true,则close
         */
        ~SocketStream();

        /**
         *  读取数据
         *      buffer 待接收数据的内存
         *      length 待接收数据的内存长度
         * return:
         *      >0 返回实际接收到的数据长度
         *      =0 socket被远端关闭
         *      <0 socket错误
         */
        virtual int read(void* buffer, size_t length) override;

        /**
         *  读取数据
         *      ba 接收数据的ByteArray
         *      length 待接收数据的内存长度
         * return:
         *      >0 返回实际接收到的数据长度
         *      =0 socket被远端关闭
         *      <0 socket错误
         */
        virtual int read(ByteArray::ptr ba, size_t length) override;

        /**
         *  写入数据
         *      buffer 待发送数据的内存
         *      length 待发送数据的内存长度
         * return:
         *      >0 返回实际接收到的数据长度
         *      =0 socket被远端关闭
         *      <0 socket错误
         */
        virtual int write(const void* buffer, size_t length) override;

        /**
         *  写入数据
         *      ba 待发送数据的ByteArray
         *      length 待发送数据的内存长度
         * return:
         *      >0 返回实际接收到的数据长度
         *      =0 socket被远端关闭
         *      <0 socket错误
         */
        virtual int write(ByteArray::ptr ba, size_t length) override;

        /**
         *  关闭socket
         */
        virtual void close() override;

        Socket::ptr getSocket() const { return m_socket;}   //返回Socket类

        /**
         *  返回是否连接
         */
        bool isConnected() const;

        Address::ptr getRemoteAddress();    //获取远端地址(本机：服务器——客户器地址     本机：客户器——服务器地址)
        
        Address::ptr getLocalAddress(); //获取本地地址(本机：服务器——服务器地址     本机：客户器——客户器地址)
        
        std::string getRemoteAddressString();   //返回远端地址的可读性字符串
        
        std::string getLocalAddressString();    //返回本地地址的可读性字符串

    protected:

        Socket::ptr m_socket;   //Socket类
        bool m_owner;   //是否主控(是否全权控制)
    };

}

#endif
