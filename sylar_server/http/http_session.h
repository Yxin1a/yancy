//  HTTPSession封装
//(HTTP服务端封装)
//  server:accept,socket
//  解析HTTP请求协议，并转发响应协议
//(响应的消息体内容自己决定)
#ifndef __SYLAR_HTTP_SESSION_H__
#define __SYLAR_HTTP_SESSION_H__

#include "../../streams/socket_stream.h"
#include "http.h"

namespace sylar
{
    namespace http
    {

        /**
         * HTTPSession封装
         */
        class HttpSession : public SocketStream
        {
        public:

            typedef std::shared_ptr<HttpSession> ptr;   //智能指针类型定义

            /**
             * 构造函数(给父类的成员函数初始化)
             *      sock Socket类型
             *      owner 是否托管
             */
            HttpSession(Socket::ptr sock, bool owner = true);

            /**
             * 接收HTTP请求，并解析HTTP协议请求
             * return:
             *      返回HttpRequest结构(HTTP请求结构体)
             */
            HttpRequest::ptr recvRequest();

            /**
             * 发送HTTP响应
             *      rsp HTTP响应(已解析好的响应)
             * return: 
             *      >0 发送成功
             *      =0 对方关闭
             *      <0 Socket异常
             */
            int sendResponse(HttpResponse::ptr rsp);
        };

    }
}

#endif