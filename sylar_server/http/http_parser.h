//      HTTP协议解析封装
//      (消息体不用解析)
#ifndef __SYLAR_HTTP_PARSER_H__
#define __SYLAR_HTTP_PARSER_H__

#include "http.h"
#include "http11_parser.h"
#include "httpclient_parser.h"

namespace sylar
{
    namespace http
    {

        /**
         *  HTTP请求解析类
         */
        class HttpRequestParser
        {
        public:
            typedef std::shared_ptr<HttpRequestParser> ptr; //HTTP请求解析类的智能指针

            /**
             *  构造函数
             */
            HttpRequestParser();

            /**
             *  解析Http请求协议
             *      data 协议文本内存
             *      len 协议文本内存长度
             * return:
             *      返回实际解析的长度,并且将已解析的数据移除
             */
            size_t execute(char* data, size_t len);

            int isFinished();   //是否解析完成

            int hasError(); //是否有错误 

            uint64_t getContentLength();    //获取消息体长度

            HttpRequest::ptr getData() const { return m_data;}  //返回HttpRequest结构(HTTP请求结构体)
            void setError(int v) { m_error = v;}    //设置错误
            const http_parser& getParser() const { return m_parser;}    //获取http_parser结构体

        public:

            static uint64_t GetHttpRequestBufferSize(); //返回HttpRequest协议解析的缓存大小(限制请求头部一个字段(一次发送的数据大小))
            static uint64_t GetHttpRequestMaxBodySize();    //返回HttpRequest协议的最大消息体大小(限制请求消息体一个字段(一次发送的数据大小))

        private:

            http_parser m_parser;   //http_parser
            HttpRequest::ptr m_data;    //HttpRequest结构(HTTP请求结构体)

            // 错误码
            // 1000: invalid method
            // 1001: invalid version
            // 1002: invalid field
            int m_error;
        };

        /**
         *  Http响应解析类
         */
        class HttpResponseParser
        {
        public:
            typedef std::shared_ptr<HttpResponseParser> ptr;    //Http响应智能指针类型

            /**
             *  构造函数
             */
            HttpResponseParser();

            /**
             *  解析HTTP响应协议
             *      data 协议数据内存
             *      len 协议数据内存大小
             *      chunck 是否在解析chunck(分块传输编码)
             * return:
             *      返回实际解析的长度,并且移除已解析的数据
             */
            size_t execute(char* data, size_t len, bool chunck);

            int isFinished();   //是否解析完成

            int hasError();     //是否有错误

            uint64_t getContentLength();    //获取消息体长度

            HttpResponse::ptr getData() const { return m_data;} //返回HttpResponse结构体(HTTP响应结构体)
            void setError(int v) { m_error = v;}    //设置错误码
            const httpclient_parser& getParser() const { return m_parser;}  //返回httpclient_parser

        public:

            static uint64_t GetHttpResponseBufferSize();    //返回HTTP响应解析缓存大小(限制响应头部一个字段(一次发送的数据大小))
            static uint64_t GetHttpResponseMaxBodySize();   //返回HTTP响应最大消息体大小(限制响应消息体一个字段(一次发送的数据大小))

        private:

            httpclient_parser m_parser; //httpclient_parser
            HttpResponse::ptr m_data;   //HttpResponse结构体(HTTP响应结构体)

            //错误码
            //1001: invalid version
            //1002: invalid field
            int m_error;
        };

    }
}

#endif