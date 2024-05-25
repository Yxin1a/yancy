#include "http_connection.h"
#include "http_parser.h"
#include "sylar_server/log.h"
//#include "sylar-master/streams/zlib_stream.h"

namespace sylar
{
    namespace http
    {

        static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

        std::string HttpResult::toString() const    //以字符串的形式输出错误码
        {
            std::stringstream ss;
            ss << "[HttpResult result=" << result
            << " error=" << error
            << " response=" << (response ? response->toString() : "nullptr")
            << "]";
            return ss.str();
        }

        //HttpConnection
        HttpConnection::HttpConnection(Socket::ptr sock, bool owner)    //初始化
            :SocketStream(sock, owner)
        {}

        HttpConnection::~HttpConnection()   //打印日志
        {
            SYLAR_LOG_DEBUG(g_logger) << "HttpConnection::~HttpConnection";
        }

        HttpResponse::ptr HttpConnection::recvResponse()    //接收HTTP响应，解析响应协议
        {
            HttpResponseParser::ptr parser(new HttpResponseParser); //HTTP响应解析类
            uint64_t buff_size = HttpResponseParser::GetHttpResponseBufferSize(); //返回HttpResponse协议解析的缓存大小(限制响应头部一个字段(一次发送的数据大小))
            //uint64_t buff_size = 100;
            std::shared_ptr<char> buffer(   //共享字符串
                    new char[buff_size + 1], [](char* ptr){
                        delete[] ptr;   //析构函数(自动释放)
                    });
            char* data = buffer.get();  //存储HTTP响应协议
            int offset = 0; //上一轮 已读取的数据中还未解析的长度(消息体——不用chunked==消息体不用解析)
            do {
                int len = read(data + offset, buff_size - offset);  //读取HTTP响应协议  (返回实际接收到的数据长度)
                if(len <= 0)
                {
                    close();
                    return nullptr;
                }
                len += offset;  //现在 已读取的数据中还未解析的长度(消息体==消息体不用解析) <=buff_size
                data[len] = '\0';
                size_t nparse = parser->execute(data, len, false);  //解析HTTP响应协议
                if(parser->hasError())
                {
                    close();
                    return nullptr;
                }
                offset = len - nparse;  //这一轮之后 已读取的数据中还未解析的长度(消息体)
                if(offset == (int)buff_size)    //出错(没有解析到)
                {
                    close();
                    return nullptr;
                }
                if(parser->isFinished())    //解析成功
                {
                    break;
                }
            } while(true);
            auto& client_parser = parser->getParser();  //返回httpclient_parser
            std::string body;   //存储消息体(响应协议)
            if(client_parser.chunked)   //设置chunked(分块传输编码)————解析剩余的数据()
            {
                int len = offset;   //已读取的数据中还未解析的长度(要解析的数据)
                do {
                    bool begin = true;  //记录有没有len(要解析的数据)是否存在没有解析的
                    do {
                        if(!begin || len == 0)  //都已解析，则读取后面包含要解析的数据分块
                        {
                            int rt = read(data + len, buff_size - len); //读取数据
                            if(rt <= 0)
                            {
                                close();
                                return nullptr;
                            }
                            len += rt;  //现在已读取的数据全长
                        }
                        data[len] = '\0';
                        size_t nparse = parser->execute(data, len, true);   //解析HTTP响应协议
                        if(parser->hasError())
                        {
                            close();
                            return nullptr;
                        }
                        len -= nparse;  //已读取的数据中还未解析的长度(消息体)
                        if(len == (int)buff_size)   //出错(没有解析到)
                        {
                            close();
                            return nullptr;
                        }
                        begin = false;
                    } while(!parser->isFinished()); //解析成功
                    //len -= 2;
                    
                    SYLAR_LOG_DEBUG(g_logger) << "content_len=" << client_parser.content_len;
                    if(client_parser.content_len + 2 <= len)    //实际消息体 < 消息体
                    {
                        body.append(data, client_parser.content_len);   //存进消息体中
                        memmove(data, data + client_parser.content_len + 2
                                , len - client_parser.content_len - 2);
                        len -= client_parser.content_len + 2;
                    }
                    else    //实际消息体 > 消息体
                    {
                        body.append(data, len);
                        int left = client_parser.content_len - len + 2;
                        while(left > 0) //把后面包含消息体分块都读取出来，存进消息体中
                        {
                            int rt = read(data, left > (int)buff_size ? (int)buff_size : left);
                            if(rt <= 0)
                            {
                                close();
                                return nullptr;
                            }
                            body.append(data, rt);  //存进消息体中
                            left -= rt;
                        }
                        body.resize(body.size() - 2);
                        len = 0;
                    }
                } while(!client_parser.chunks_done);
            }
            else    //不是chunked(分块传输编码)————消息体为响应协议
            {
                int64_t length = parser->getContentLength();    //获取消息体长度
                if(length > 0)
                {
                    body.resize(length);

                    int len = 0;    //实际消息体
                    if(length >= offset)    //消息体长度 > 已读取的数据中还未解析的长度(消息体)
                    {
                        memcpy(&body[0], data, offset);
                        len = offset;
                    }
                    else
                    {
                        memcpy(&body[0], data, length);
                        len = length;
                    }
                    length -= offset;   //还未读取的消息体数据
                    if(length > 0)
                    {
                        if(readFixSize(&body[len], length) <= 0)    //读取剩下的消息体数据
                        {
                            close();
                            return nullptr;
                        }
                    }
                }
            }
            if(!body.empty())
            {
                // auto content_encoding = parser->getData()->getHeader("content-encoding");
                // SYLAR_LOG_DEBUG(g_logger) << "content_encoding: " << content_encoding
                //     << " size=" << body.size();
                // if(strcasecmp(content_encoding.c_str(), "gzip") == 0)
                // {
                //     auto zs = ZlibStream::CreateGzip(false);
                //     zs->write(body.c_str(), body.size());
                //     zs->flush();
                //     zs->getResult().swap(body);
                // } else if(strcasecmp(content_encoding.c_str(), "deflate") == 0)
                // {
                //     auto zs = ZlibStream::CreateDeflate(false);
                //     zs->write(body.c_str(), body.size());
                //     zs->flush();
                //     zs->getResult().swap(body);
                // }
                parser->getData()->setBody(body);
            }
            return parser->getData();
        }

        int HttpConnection::sendRequest(HttpRequest::ptr rsp)   //发送HTTP请求
        {
            std::stringstream ss;
            ss << *rsp;
            std::string data = ss.str();
            //std::cout << ss.str() << std::endl;
            return writeFixSize(data.c_str(), data.size()); //发送HTTP响应
        }

        HttpResult::ptr HttpConnection::DoGet(const std::string& url    //发送HTTP的GET请求(字符串uri)
                                    , uint64_t timeout_ms
                                    , const std::map<std::string, std::string>& headers
                                    , const std::string& body)
        {
            Uri::ptr uri = Uri::Create(url);    //创建Uri对象，并解析uri(网址)
            if(!uri)
            {
                return std::make_shared<HttpResult>((int)HttpResult::Error::INVALID_URL //创建HttpResult HTTP响应结果(非法URL)
                        , nullptr, "invalid url: " + url);  //make_shared功能是在动态内存中分配一个对象并初始化它，返回指向此对象的shared_ptr
            }
            return DoGet(uri, timeout_ms, headers, body);   //发送HTTP的GET请求(结构体uri)
        }

        HttpResult::ptr HttpConnection::DoGet(Uri::ptr uri  //发送HTTP的GET请求(结构体uri)
                                    , uint64_t timeout_ms
                                    , const std::map<std::string, std::string>& headers
                                    , const std::string& body)
        {
            return DoRequest(HttpMethod::GET, uri, timeout_ms, headers, body);  //发送HTTP请求
        }

        HttpResult::ptr HttpConnection::DoPost(const std::string& url   //发送HTTP的POST请求(字符串uri)
                                    , uint64_t timeout_ms
                                    , const std::map<std::string, std::string>& headers
                                    , const std::string& body)
        {
            Uri::ptr uri = Uri::Create(url);    //创建Uri对象，并解析uri(网址)
            if(!uri)
            {
                return std::make_shared<HttpResult>((int)HttpResult::Error::INVALID_URL //创建HttpResult HTTP响应结果(非法URL)
                        , nullptr, "invalid url: " + url);  //make_shared功能是在动态内存中分配一个对象并初始化它，返回指向此对象的shared_ptr
            }
            return DoPost(uri, timeout_ms, headers, body);  //发送HTTP的POST请求(结构体uri)
        }

        HttpResult::ptr HttpConnection::DoPost(Uri::ptr uri //发送HTTP的POST请求(结构体uri)
                                    , uint64_t timeout_ms
                                    , const std::map<std::string, std::string>& headers
                                    , const std::string& body)
        {
            return DoRequest(HttpMethod::POST, uri, timeout_ms, headers, body); //发送HTTP请求
        }

        HttpResult::ptr HttpConnection::DoRequest(HttpMethod method //发送HTTP请求(字符串uri)
                                    , const std::string& url
                                    , uint64_t timeout_ms
                                    , const std::map<std::string, std::string>& headers
                                    , const std::string& body)
        {
            Uri::ptr uri = Uri::Create(url);    //创建Uri对象，并解析uri(网址)
            if(!uri)
            {
                return std::make_shared<HttpResult>((int)HttpResult::Error::INVALID_URL //创建HttpResult HTTP响应结果(非法URL)
                        , nullptr, "invalid url: " + url);  //make_shared功能是在动态内存中分配一个对象并初始化它，返回指向此对象的shared_ptr
            }
            return DoRequest(method, uri, timeout_ms, headers, body);   //发送HTTP请求(结构体uri)
        }

        HttpResult::ptr HttpConnection::DoRequest(HttpMethod method //发送HTTP请求(结构体uri)，请求协议内容转化成请求结构体
                                    , Uri::ptr uri
                                    , uint64_t timeout_ms
                                    , const std::map<std::string, std::string>& headers
                                    , const std::string& body)
        {
            HttpRequest::ptr req = std::make_shared<HttpRequest>(); //创建HTTP请求结构体
                                                                    //make_shared功能是在动态内存中分配一个对象并初始化它，返回指向此对象的shared_ptr
            req->setPath(uri->getPath());   //设置HTTP请求的路径
            req->setQuery(uri->getQuery()); //设置HTTP请求的查询参数
            req->setFragment(uri->getFragment());   //设置HTTP请求的Fragment(片段)
            req->setMethod(method); //设置请求方法
            bool has_host = false;  //判断headers是否没有host
            for(auto& i : headers)  //遍历HTTP请求头部参数
            {
                if(strcasecmp(i.first.c_str(), "connection") == 0)  //存在connection
                {   //strcasecmp判断字符串是否相等(不管大小写)
                    if(strcasecmp(i.second.c_str(), "keep-alive") == 0) //不是长连接
                    {
                        req->setClose(false);   //设置不自动关闭
                    }
                    continue;
                }

                if(!has_host && strcasecmp(i.first.c_str(), "host") == 0)   //存在host
                {
                    has_host = !i.second.empty();
                }

                req->setHeader(i.first, i.second);  //设置HTTP请求的头部参数
            }
            if(!has_host)   //存在host
            {
                req->setHeader("Host", uri->getHost()); //设置HTTP请求的头部参数
            }
            req->setBody(body); //设置消息体
            return DoRequest(req, uri, timeout_ms); //发送HTTP请求
        }

        HttpResult::ptr HttpConnection::DoRequest(HttpRequest::ptr req  //发送HTTP请求(请求结构体、结构体uri)
                                    , Uri::ptr uri
                                    , uint64_t timeout_ms)
        {
            bool is_ssl = uri->getScheme() == "https";  //判断请求协议是否是https
            Address::ptr addr = uri->createAddress();   //根据uri类，获取Address
            if(!addr)
            {
                return std::make_shared<HttpResult>((int)HttpResult::Error::INVALID_HOST    //创建HttpResult HTTP响应结果(无法解析HOST)
                        , nullptr, "invalid host: " + uri->getHost());  //make_shared功能是在动态内存中分配一个对象并初始化它，返回指向此对象的shared_ptr
            }
            //Socket::ptr sock = is_ssl ? SSLSocket::CreateTCP(addr) : Socket::CreateTCP(addr);
            Socket::ptr sock =Socket::CreateTCP(addr);  //客户端    处理服务器，不包含IP地址
            if(!sock)
            {
                return std::make_shared<HttpResult>((int)HttpResult::Error::CREATE_SOCKET_ERROR //创建HttpResult HTTP响应结果(创建Socket失败)
                        , nullptr, "create socket fail: " + addr->toString()
                                + " errno=" + std::to_string(errno) //make_shared功能是在动态内存中分配一个对象并初始化它，返回指向此对象的shared_ptr
                                + " errstr=" + std::string(strerror(errno)));
            }
            if(!sock->connect(addr))    //客户端连接服务器地址
            {
                return std::make_shared<HttpResult>((int)HttpResult::Error::CONNECT_FAIL    //创建HttpResult HTTP响应结果(连接失败)
                        , nullptr, "connect fail: " + addr->toString());    //make_shared功能是在动态内存中分配一个对象并初始化它，返回指向此对象的shared_ptr
            }
            sock->setRecvTimeout(timeout_ms);   //设置读取数据的超时时间
            HttpConnection::ptr conn = std::make_shared<HttpConnection>(sock);  //make_shared功能是在动态内存中分配一个对象并初始化它，返回指向此对象的shared_ptr
            //  嵌套HTTP客户端类
            int rt = conn->sendRequest(req);    //发送HTTP请求
            if(rt == 0) //连接被对端关闭
            {
                return std::make_shared<HttpResult>((int)HttpResult::Error::SEND_CLOSE_BY_PEER  //创建HttpResult HTTP响应结果(连接被对端关闭)
                        , nullptr, "send request closed by peer: " + addr->toString());
            }
            if(rt < 0)  //发送请求产生Socket错误
            {
                return std::make_shared<HttpResult>((int)HttpResult::Error::SEND_SOCKET_ERROR   //创建HttpResult HTTP响应结果(发送请求产生Socket错误)
                            , nullptr, "send request socket error errno=" + std::to_string(errno)
                            + " errstr=" + std::string(strerror(errno)));
            }
            auto rsp = conn->recvResponse();    //接收HTTP响应
            if(!rsp)
            {
                return std::make_shared<HttpResult>((int)HttpResult::Error::TIMEOUT //创建HttpResult HTTP响应结果(超时)
                            , nullptr, "recv response timeout: " + addr->toString()
                            + " timeout_ms:" + std::to_string(timeout_ms));
            }
            return std::make_shared<HttpResult>((int)HttpResult::Error::OK, rsp, "ok"); //正常
        }

        //HttpConnectionPool
        HttpConnectionPool::ptr HttpConnectionPool::Create(const std::string& uri
                                                        ,const std::string& vhost
                                                        ,uint32_t max_size
                                                        ,uint32_t max_alive_time
                                                        ,uint32_t max_request)
        {
            Uri::ptr turi = Uri::Create(uri);   //创建Uri对象，并解析uri(网址)
            if(!turi)
            {
                SYLAR_LOG_ERROR(g_logger) << "invalid uri=" << uri;
            }
            return std::make_shared<HttpConnectionPool>(turi->getHost()
                    , vhost, turi->getPort(), turi->getScheme() == "https"
                    , max_size, max_alive_time, max_request);
        }

        HttpConnectionPool::HttpConnectionPool(const std::string& host  //初始化
                                                ,const std::string& vhost
                                                ,uint32_t port
                                                ,bool is_https
                                                ,uint32_t max_size
                                                ,uint32_t max_alive_time
                                                ,uint32_t max_request)
            :m_host(host)
            ,m_vhost(vhost)
            ,m_port(port ? port : (is_https ? 443 : 80))
            ,m_maxSize(max_size)
            ,m_maxAliveTime(max_alive_time)
            ,m_maxRequest(max_request)
            ,m_isHttps(is_https)
        {}

        HttpConnection::ptr HttpConnectionPool::getConnection() //从连接池取出链接，没有，则自动根据host创建链接
        {
            uint64_t now_ms = sylar::GetCurrentMS();    //获取当前时间的毫秒级
            std::vector<HttpConnection*> invalid_conns; //存储失效，丢弃的链接
            HttpConnection* ptr = nullptr;  //存储获取的链接
            MutexType::Lock lock(m_mutex);  //锁m_cons(list)
            while(!m_conns.empty())
            {
                auto conn = *m_conns.begin();
                m_conns.pop_front();    //移除
                if(!conn->isConnected())    //不是连接状态
                {
                    invalid_conns.push_back(conn);
                    continue;
                }
                if((conn->m_createTime + m_maxAliveTime) > now_ms)  //超过链接存在时间上限
                {
                    invalid_conns.push_back(conn);
                    continue;
                }
                ptr = conn;
                break;
            }
            lock.unlock();
            for(auto i : invalid_conns) //释放失效，丢弃的链接
            {
                delete i;
            }
            m_total -= invalid_conns.size();    //当前连接池连接条数

            if(!ptr)    //不存在，则自动根据host创建链接
            {
                IPAddress::ptr addr = Address::LookupAnyIPAddress(m_host);  //通过host地址返回对应条件的任意IPAddress
                if(!addr)
                {
                    SYLAR_LOG_ERROR(g_logger) << "get addr fail: " << m_host;
                    return nullptr;
                }
                addr->setPort(m_port);
                // Socket::ptr sock = m_isHttps ? SSLSocket::CreateTCP(addr) : Socket::CreateTCP(addr);
                Socket::ptr sock = Socket::CreateTCP(addr);
                if(!sock)
                {
                    SYLAR_LOG_ERROR(g_logger) << "create sock fail: " << *addr;
                    return nullptr;
                }
                if(!sock->connect(addr))    //客户端连接服务器地址 
                {
                    SYLAR_LOG_ERROR(g_logger) << "sock connect fail: " << *addr;
                    return nullptr;
                }

                ptr = new HttpConnection(sock);
                ++m_total;
            }
            return HttpConnection::ptr(ptr, std::bind(&HttpConnectionPool::ReleasePtr
                                    , std::placeholders::_1, this));
        }

        void HttpConnectionPool::ReleasePtr(HttpConnection* ptr, HttpConnectionPool* pool)  //释放连接池指针()
        {
            ++ptr->m_request;   //链接请求次数
            if(!ptr->isConnected()
                    || ((ptr->m_createTime + pool->m_maxAliveTime) >= sylar::GetCurrentMS())
                    || (ptr->m_request >= pool->m_maxRequest))  //不处于连接状态、超过链接存在时间上限、超过链接请求次数上限    释放
            {
                delete ptr;
                --pool->m_total;
                return;
            }
            MutexType::Lock lock(pool->m_mutex);
            pool->m_conns.push_back(ptr);   //判断错误，没问题
        }

        HttpResult::ptr HttpConnectionPool::doGet(const std::string& url    //发送HTTP的GET请求(字符串uri)
                                , uint64_t timeout_ms
                                , const std::map<std::string, std::string>& headers
                                , const std::string& body)
        {
            return doRequest(HttpMethod::GET, url, timeout_ms, headers, body);  //发送HTTP请求(字符串uri)
        }

        HttpResult::ptr HttpConnectionPool::doGet(Uri::ptr uri  //发送HTTP的GET请求(结构体uri)
                                        , uint64_t timeout_ms
                                        , const std::map<std::string, std::string>& headers
                                        , const std::string& body)
        {
            //结构体uri 转化成 字符串uri
            //path?query#fragment
            std::stringstream ss;
            ss << uri->getPath()
            << (uri->getQuery().empty() ? "" : "?")
            << uri->getQuery()
            << (uri->getFragment().empty() ? "" : "#")
            << uri->getFragment();
            return doGet(ss.str(), timeout_ms, headers, body);  //发送HTTP的GET请求(字符串uri)
        }

        HttpResult::ptr HttpConnectionPool::doPost(const std::string& url   //发送HTTP的POST请求(字符串uri)
                                        , uint64_t timeout_ms
                                        , const std::map<std::string, std::string>& headers
                                        , const std::string& body)
        {
            return doRequest(HttpMethod::POST, url, timeout_ms, headers, body); //发送HTTP请求(字符串uri)
        }

        HttpResult::ptr HttpConnectionPool::doPost(Uri::ptr uri //发送HTTP的POST请求(结构体uri)
                                        , uint64_t timeout_ms
                                        , const std::map<std::string, std::string>& headers
                                        , const std::string& body)
        {
            //结构体uri 转化成 字符串uri
            //path?query#fragment
            std::stringstream ss;
            ss << uri->getPath()
            << (uri->getQuery().empty() ? "" : "?")
            << uri->getQuery()
            << (uri->getFragment().empty() ? "" : "#")
            << uri->getFragment();
            return doPost(ss.str(), timeout_ms, headers, body); //发送HTTP的POST请求(字符串uri)
        }

        HttpResult::ptr HttpConnectionPool::doRequest(HttpMethod method //发送HTTP请求(字符串uri)
                                            , const std::string& url
                                            , uint64_t timeout_ms
                                            , const std::map<std::string, std::string>& headers
                                            , const std::string& body)
        {
            HttpRequest::ptr req = std::make_shared<HttpRequest>();
            req->setPath(url);  //设置路径
            req->setMethod(method); //设置请求方法
            req->setClose(false);   //设置是否自动关闭
            bool has_host = false;
            for(auto& i : headers)  //遍历HTTP请求头部参数
            {
                if(strcasecmp(i.first.c_str(), "connection") == 0)  //存在connection
                {   //strcasecmp判断字符串是否相等(不管大小写)
                    if(strcasecmp(i.second.c_str(), "keep-alive") == 0) //不是长连接
                    {
                        req->setClose(false);       //设置不自动关闭
                    }
                    continue;
                }

                if(!has_host && strcasecmp(i.first.c_str(), "host") == 0)   //存在host
                {
                    has_host = !i.second.empty();
                }

                req->setHeader(i.first, i.second);  //设置HTTP请求的头部参数
            }
            if(!has_host)   //host存在
            {
                if(m_vhost.empty())
                {
                    req->setHeader("Host", m_host); //设置HTTP请求的头部参数
                }
                else
                {
                    req->setHeader("Host", m_vhost);    //设置HTTP请求的头部参数
                }
            }
            req->setBody(body);
            return doRequest(req, timeout_ms);
        }

        HttpResult::ptr HttpConnectionPool::doRequest(HttpMethod method //发送HTTP请求(结构体uri)
                                            , Uri::ptr uri
                                            , uint64_t timeout_ms
                                            , const std::map<std::string, std::string>& headers
                                            , const std::string& body)
        {
            //结构体uri 转化成 字符串uri
            //path?query#fragment
            std::stringstream ss;
            ss << uri->getPath()
            << (uri->getQuery().empty() ? "" : "?")
            << uri->getQuery()
            << (uri->getFragment().empty() ? "" : "#")
            << uri->getFragment();
            return doRequest(method, ss.str(), timeout_ms, headers, body);  //发送HTTP请求(字符串uri)
        }

        HttpResult::ptr HttpConnectionPool::doRequest(HttpRequest::ptr req  //发送HTTP请求
                                                , uint64_t timeout_ms)
        {
            auto conn = getConnection();    //从连接池取出链接
            if(!conn)
            {
                return std::make_shared<HttpResult>((int)HttpResult::Error::POOL_GET_CONNECTION //从连接池中取连接失败
                        , nullptr, "pool host:" + m_host + " port:" + std::to_string(m_port));
            }
            auto sock = conn->getSocket();  //返回Socket类
            if(!sock)
            {
                return std::make_shared<HttpResult>((int)HttpResult::Error::POOL_INVALID_CONNECTION //无效的连接
                        , nullptr, "pool host:" + m_host + " port:" + std::to_string(m_port));
            }
            sock->setRecvTimeout(timeout_ms);   //设置读取超时时间
            int rt = conn->sendRequest(req);    //发送请求
            if(rt == 0) //连接被对端关闭
            {
                return std::make_shared<HttpResult>((int)HttpResult::Error::SEND_CLOSE_BY_PEER  //连接被对端关闭
                        , nullptr, "send request closed by peer: " + sock->getRemoteAddress()->toString());
            }
            if(rt < 0)  //发送请求产生Socket错误
            {
                return std::make_shared<HttpResult>((int)HttpResult::Error::SEND_SOCKET_ERROR   //发送请求产生Socket错误
                            , nullptr, "send request socket error errno=" + std::to_string(errno)
                            + " errstr=" + std::string(strerror(errno)));
            }
            auto rsp = conn->recvResponse();    //读取响应
            if(!rsp)
            {
                return std::make_shared<HttpResult>((int)HttpResult::Error::TIMEOUT //超时
                            , nullptr, "recv response timeout: " + sock->getRemoteAddress()->toString()
                            + " timeout_ms:" + std::to_string(timeout_ms));
            }
            return std::make_shared<HttpResult>((int)HttpResult::Error::OK, rsp, "ok"); //正常
        }

    }
}
