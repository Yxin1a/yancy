#include"http_parser.h"
#include"../log.h"
#include"../config.h"
#include<string.h>
#include<stdlib.h>

namespace sylar
{
    namespace http
    {
        //全局

        static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

        //限制请求头部一个字段(一次发送的数据大小)为4K————超出则数据有问题
        static sylar::ConfigVar<uint64_t>::ptr g_http_request_buffer_size =
            sylar::Config::Lookup("http.request.buffer_size"
                        ,(uint64_t)(4 * 1024), "http request buffer size");

        //限制请求消息体一个字段(一次发送的数据大小)为64M————超出则数据有问题
        static sylar::ConfigVar<uint64_t>::ptr g_http_request_max_body_size =
            sylar::Config::Lookup("http.request.max_body_size"
                        ,(uint64_t)(64 * 1024 * 1024), "http request max body size");

        //限制响应头部一个字段(一次发送的数据大小)为4K————超出则数据有问题
        static sylar::ConfigVar<uint64_t>::ptr g_http_response_buffer_size =
            sylar::Config::Lookup("http.response.buffer_size"
                        ,(uint64_t)(4 * 1024), "http response buffer size");

        //限制响应消息体一个字段(一次发送的数据大小)为64M————超出则数据有问题
        static sylar::ConfigVar<uint64_t>::ptr g_http_response_max_body_size =
            sylar::Config::Lookup("http.response.max_body_size"
                        ,(uint64_t)(64 * 1024 * 1024), "http response max body size");

        static uint64_t s_http_request_buffer_size = 0;     //限制请求头部一个字段(一次发送的数据大小)为4K
        static uint64_t s_http_request_max_body_size = 0;   //限制请求消息体一个字段(一次发送的数据大小)为64M
        static uint64_t s_http_response_buffer_size = 0;    //限制响应头部一个字段(一次发送的数据大小)为4K
        static uint64_t s_http_response_max_body_size = 0;  //限制响应消息体一个字段(一次发送的数据大小)为64M

        namespace
        {
            struct _RequestSizeIniter   //初始化
            {
                _RequestSizeIniter()
                {
                    s_http_request_buffer_size = g_http_request_buffer_size->getValue();    //获取当前参数值
                    s_http_request_max_body_size = g_http_request_max_body_size->getValue();    //获取当前参数值
                    s_http_response_buffer_size = g_http_response_buffer_size->getValue();  //获取当前参数值
                    s_http_response_max_body_size = g_http_response_max_body_size->getValue();  //获取当前参数值

                    g_http_request_buffer_size->addListener(    //添加变化回调函数
                            [](const uint64_t& ov, const uint64_t& nv){
                            s_http_request_buffer_size = nv;
                    });

                    g_http_request_max_body_size->addListener(  //添加变化回调函数
                            [](const uint64_t& ov, const uint64_t& nv){
                            s_http_request_max_body_size = nv;
                    });

                    g_http_response_buffer_size->addListener(   //添加变化回调函数
                            [](const uint64_t& ov, const uint64_t& nv){
                            s_http_response_buffer_size = nv;
                    });

                    g_http_response_max_body_size->addListener( //添加变化回调函数
                            [](const uint64_t& ov, const uint64_t& nv){
                            s_http_response_max_body_size = nv;
                    });
                }
            };
            static _RequestSizeIniter _init;    //编译时执行(全局)
        }

        //HttpRequestParser

        /**
         *  data HTTP请求解析类
         *  at HTTP请求协议的各个部分
         *  length at的长度
        */
        void on_request_method(void *data, const char *at, size_t length)   //设置HTTP请求协议的URL
        {
            HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);  //转换成HttpRequestParser  HTTP请求解析类
            HttpMethod m = CharsToHttpMethod(at);   //将字符串指针转换成HTTP请求方法枚举

            if(m == HttpMethod::INVALID_METHOD) //HTTP请求方法枚举类没有
            {
                SYLAR_LOG_WARN(g_logger) << "invalid http request method: "
                    << std::string(at, length);
                parser->setError(1000);
                return;
            }
            parser->getData()->setMethod(m);    //设置HTTP请求的方法(名称=序号  GET=1)
        }

        void on_request_uri(void *data, const char *at, size_t length)
        {}

        void on_request_fragment(void *data, const char *at, size_t length) //设置HTTP请求协议的请求fragment
        {
            HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);  //转换成HttpRequestParser  HTTP请求解析类
            parser->getData()->setFragment(std::string(at, length));
        }

        void on_request_path(void *data, const char *at, size_t length)     //设置HTTP请求协议的请求对应的路径
        {
            HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);  //转换成HttpRequestParser  HTTP请求解析类
            parser->getData()->setPath(std::string(at, length));
        }

        void on_request_query(void *data, const char *at, size_t length)    //设置HTTP请求协议的请求参数
        {
            HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);  //转换成HttpRequestParser  HTTP请求解析类
            parser->getData()->setQuery(std::string(at, length));
        }

        void on_request_version(void *data, const char *at, size_t length)  //设置HTTP请求协议的HTTP版本
        {
            HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);  //转换成HttpRequestParser  HTTP请求解析类
            uint8_t v = 0;  //HTTP版本
            if(strncmp(at, "HTTP/1.1", length) == 0)    //strncmp判断字符串是否相等
            {
                v = 0x11;
            }
            else if(strncmp(at, "HTTP/1.0", length) == 0)
            {
                v = 0x10;
            }
            else
            {
                SYLAR_LOG_WARN(g_logger) << "invalid http request version: "
                    << std::string(at, length);
                parser->setError(1001);
                return;
            }
            parser->getData()->setVersion(v);   //设置HTTP版本
        }

        void on_request_header_done(void *data, const char *at, size_t length)
        {}

        /**
         *  data HTTP请求解析类
         *  field 头部参数key
         *  flen field的长度
         *  value 头部参数
         *  vlen value的长度
        */
        void on_request_http_field(void *data, const char *field, size_t flen   //设置HTTP请求协议的头部参数
                                ,const char *value, size_t vlen)
        {
            HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);  //转换成HttpRequestParser  HTTP请求解析类
            if(flen == 0)   //key为0
            {
                SYLAR_LOG_WARN(g_logger) << "invalid http request field length == 0";
                return;
            }
            parser->getData()->setHeader(std::string(field, flen)   //设置HTTP请求的头部参数
                                        ,std::string(value, vlen));
        }

        HttpRequestParser::HttpRequestParser()  //初始化    m_error m_data m_parser
            :m_error(0)
        {
            m_data.reset(new sylar::http::HttpRequest);
            http_parser_init(&m_parser);    //创建
            m_parser.request_method = on_request_method;
            m_parser.request_uri = on_request_uri;
            m_parser.fragment = on_request_fragment;
            m_parser.request_path = on_request_path;
            m_parser.query_string = on_request_query;
            m_parser.http_version = on_request_version;
            m_parser.header_done = on_request_header_done;
            m_parser.http_field = on_request_http_field;
            m_parser.data = this;
        }
        
        //1: 成功
        //-1: 有错误
        //>0: 已处理的字节数，且data有效数据为len - v;
        size_t HttpRequestParser::execute(char* data, size_t len)   //解析Http请求协议(返回实际解析的长度,并且将已解析的数据移除)
        {
            size_t offset = http_parser_execute(&m_parser, data, len, 0);   //解析Http请求协议,返回实际解析的长度
            memmove(data, data + offset, (len - offset));   //将已解析的数据移除    memmove()移动内存块
            return offset;
        }

        int HttpRequestParser::isFinished()   //是否解析完成
        {
            return http_parser_finish(&m_parser);
        }
        
        int HttpRequestParser::hasError() //是否有错误 
        {
            return m_error || http_parser_has_error(&m_parser);
        }
        
        uint64_t HttpRequestParser::getContentLength()    //获取消息体长度
        {
            return m_data->getHeaderAs<uint64_t>("content-length", 0);  //获取HTTP请求的头部参数中的消息体的长度
        }
        
        uint64_t HttpRequestParser::GetHttpRequestBufferSize() //返回HttpRequest协议解析的缓存大小(限制请求头部一个字段(一次发送的数据大小))
        {
            return s_http_request_buffer_size;
        }
        
        uint64_t HttpRequestParser::GetHttpRequestMaxBodySize()    //返回HttpRequest协议的最大消息体大小(限制请求消息体一个字段(一次发送的数据大小))
        {
            return s_http_request_max_body_size;
        }
        
        //HttpResponseParser

        /**
         *  data HTTP响应解析类
         *  at HTTP响应协议的各个部分
         *  length at的长度
        */
        void on_response_reason(void *data, const char *at, size_t length)  //设置HTTP响应协议的响应原因
        {
            HttpResponseParser* parser = static_cast<HttpResponseParser*>(data);    //转换成HttpResponseParser  Http响应解析类
            parser->getData()->setReason(std::string(at, length));  //设置响应原因
        }

        void on_response_status(void *data, const char *at, size_t length)  //设置HTTP响应协议的响应状态
        {
            HttpResponseParser* parser = static_cast<HttpResponseParser*>(data);    //转换成HttpResponseParser  Http响应解析类
            HttpStatus status = (HttpStatus)(atoi(at)); //atoi()是把字符串转换成整型数
            parser->getData()->setStatus(status);   //设置响应状态
        }

        void on_response_chunk(void *data, const char *at, size_t length)
        {}

        void on_response_version(void *data, const char *at, size_t length) //设置HTTP响应协议的HTTP版本
        {
            HttpResponseParser* parser = static_cast<HttpResponseParser*>(data);    //转换成HttpResponseParser  Http响应解析类
            uint8_t v = 0;  //HTTP版本
            if(strncmp(at, "HTTP/1.1", length) == 0)    //判断字符串是否相等
            {
                v = 0x11;
            }
            else if(strncmp(at, "HTTP/1.0", length) == 0)
            {
                v = 0x10;
            }
            else
            {
                SYLAR_LOG_WARN(g_logger) << "invalid http response version: "
                    << std::string(at, length);
                parser->setError(1001);
                return;
            }

            parser->getData()->setVersion(v);   //设置HTTP版本
        }

        void on_response_header_done(void *data, const char *at, size_t length)
        {}

        void on_response_last_chunk(void *data, const char *at, size_t length)
        {}
        
        /**
         *  data HTTP请求解析类
         *  field 头部参数key
         *  flen field的长度
         *  value 头部参数
         *  vlen value的长度
        */
        void on_response_http_field(void *data, const char *field, size_t flen  //设置HTTP响应协议的响应头部参数
                                ,const char *value, size_t vlen)
        {
            HttpResponseParser* parser = static_cast<HttpResponseParser*>(data);    //转换成HttpResponseParser  Http响应解析类
            if(flen == 0)
            {
                SYLAR_LOG_WARN(g_logger) << "invalid http response field length == 0";
                return;
            }
            parser->getData()->setHeader(std::string(field, flen)   //设置响应头部参数
                                        ,std::string(value, vlen));
        }

        HttpResponseParser::HttpResponseParser()    //初始化    m_error m_data m_parser
            :m_error(0)
        {
            m_data.reset(new sylar::http::HttpResponse);
            httpclient_parser_init(&m_parser);  //创建
            m_parser.reason_phrase = on_response_reason;
            m_parser.status_code = on_response_status;
            m_parser.chunk_size = on_response_chunk;
            m_parser.http_version = on_response_version;
            m_parser.header_done = on_response_header_done;
            m_parser.last_chunk = on_response_last_chunk;
            m_parser.http_field = on_response_http_field;
            m_parser.data = this;
        }

        size_t HttpResponseParser::execute(char* data, size_t len, bool chunck) //解析HTTP响应协议(返回实际解析的长度,并且将已解析的数据移除)
        {
            if(chunck)  //不存在
            {
                httpclient_parser_init(&m_parser);
            }
            size_t offset = httpclient_parser_execute(&m_parser, data, len, 0); //解析Http响应协议,返回实际解析的长度

            memmove(data, data + offset, (len - offset));   //将已解析的数据移除    memmove()移动内存块
            return offset;
        }
        
        int HttpResponseParser::isFinished()   //是否解析完成
        {
            return httpclient_parser_finish(&m_parser);
        }
        
        int HttpResponseParser::hasError()     //是否有错误
        {
            return m_error || httpclient_parser_has_error(&m_parser);
        }
        
        uint64_t HttpResponseParser::getContentLength()    //获取消息体长度
        {
            return m_data->getHeaderAs<uint64_t>("content-length", 0);  //获取HTTP请求的头部参数中的消息体的长度
        }
        
        uint64_t HttpResponseParser::GetHttpResponseBufferSize()    //返回HTTP响应解析缓存大小(限制响应头部一个字段(一次发送的数据大小))
        {
            return s_http_response_buffer_size;
        }
        
        uint64_t HttpResponseParser::GetHttpResponseMaxBodySize()   //返回HTTP响应最大消息体大小(限制响应消息体一个字段(一次发送的数据大小))
        {
            return s_http_response_max_body_size;
        }
        
    }
}