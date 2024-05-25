#include"http.h"
#include"../util.h"

namespace sylar
{
    namespace http
    {
        //全局
        HttpMethod StringToHttpMethod(const std::string& m)    //将字符串方法名转成HTTP请求方法枚举
        {
        #define XX(num, name, string) \
            if(strcmp(#string, m.c_str()) == 0) \
            { \
                return HttpMethod::name; \
            }

            HTTP_METHOD_MAP(XX);
        #undef XX
            return HttpMethod::INVALID_METHOD;
        }

        HttpMethod CharsToHttpMethod(const char* m)    //将字符串指针转换成HTTP请求方法枚举
        {
        #define XX(num, name, string) \
            if(strncmp(#string, m, strlen(#string)) == 0) \
            { \
                return HttpMethod::name; \
            }

            HTTP_METHOD_MAP(XX);
        #undef XX
            return HttpMethod::INVALID_METHOD;
        }
        
        static const char* s_method_string[] =  //HTTP方法数组(序号=下标   名称(描述)=下标对应的值)
        {
            //#define XX(num, name, string) #string, 的返回值是#string
        #define XX(num, name, string) #string,
            HTTP_METHOD_MAP(XX)
        #undef XX
        };

        const char* HttpMethodToString(const HttpMethod& m)    //将HTTP请求方法枚举转换成字符串
        {
            uint32_t idx = (uint32_t)m;
            if(idx >= (sizeof(s_method_string) / sizeof(s_method_string[0])))   //idx > 序号最大值
            {
                return "<unknown>";
            }
            return s_method_string[idx];
        }
        
        const char* HttpStatusToString(const HttpStatus& s)    //将HTTP响应状态枚举转换成字符串
        {
            switch(s)
            {
        #define XX(code, name, msg) \
                case HttpStatus::name: \
                    return #msg;
                HTTP_STATUS_MAP(XX);
        #undef XX
                default:
                    return "<unknown>";
            }
        }

        bool CaseInsensitiveLess::operator()(const std::string& lhs, const std::string& rhs) const   //忽略大小写比较字符串是否相等
        {
            return strcasecmp(lhs.c_str(), rhs.c_str()) < 0;    //判断字符串是否相等的函数，忽略大小写
        }

        //HttpRequest

        HttpRequest::HttpRequest(uint8_t version, bool close)   //构造函数  初始化
            :m_method(HttpMethod::GET)
            ,m_version(version)
            ,m_close(close)
            ,m_websocket(false)
            ,m_parserParamFlag(0)
            ,m_path("/")    //本目录
        {}

        // std::shared_ptr<HttpResponse> HttpRequest::createResponse()
        // {
        //     HttpResponse::ptr rsp(new HttpResponse(getVersion()
        //                     ,isClose()));
        //     return rsp;
        // }

        std::string HttpRequest::getHeader(const std::string& key, const std::string& def) const    //获取HTTP请求的头部参数
        {
            auto it = m_headers.find(key);
            return it == m_headers.end() ? def : it->second;
        }

        std::string HttpRequest::getParam(const std::string& key, const std::string& def)   //获取HTTP请求的请求参数
        {
            //initQueryParam();
            //initBodyParam();
            auto it = m_params.find(key);
            return it == m_params.end() ? def : it->second;
        }

        std::string HttpRequest::getCookie(const std::string& key, const std::string& def)  //获取HTTP请求的Cookie参数
        {
            //initCookies();
            auto it = m_cookies.find(key);
            return it == m_cookies.end() ? def : it->second;
        }

        void HttpRequest::setHeader(const std::string& key, const std::string& val) //设置HTTP请求的头部参数
        {
            m_headers[key] = val;
        }

        void HttpRequest::setParam(const std::string& key, const std::string& val)  //设置HTTP请求的请求参数
        {
            m_params[key] = val;
        }

        void HttpRequest::setCookie(const std::string& key, const std::string& val) //设置HTTP请求的Cookie参数
        {
            m_cookies[key] = val;
        }

        void HttpRequest::delHeader(const std::string& key) //删除HTTP请求的头部参数
        {
            m_headers.erase(key);
        }

        void HttpRequest::delParam(const std::string& key)  //删除HTTP请求的请求参数
        {
            m_params.erase(key);
        }

        void HttpRequest::delCookie(const std::string& key) //删除HTTP请求的Cookie参数
        {
            m_cookies.erase(key);
        }

        bool HttpRequest::hasHeader(const std::string& key, std::string* val)   //判断HTTP请求的头部参数是否存在
        {
            auto it = m_headers.find(key);  //寻找
            if(it == m_headers.end())   //不存在
            {
                return false;
            }
            if(val)
            {
                *val = it->second;  
            }
            return true;
        }

        bool HttpRequest::hasParam(const std::string& key, std::string* val)    //判断HTTP请求的请求参数是否存在
        {
            //initQueryParam();
            //initBodyParam();
            auto it = m_params.find(key);   //寻找
            if(it == m_params.end())    //不存在
            {
                return false;
            }
            if(val)
            {
                *val = it->second;
            }
            return true;
        }

        bool HttpRequest::hasCookie(const std::string& key, std::string* val)   //判断HTTP请求的Cookie参数是否存在
        {
            //initCookies();
            auto it = m_cookies.find(key);  //寻找
            if(it == m_cookies.end())   //不存在
            {
                return false;
            }
            if(val)
            {
                *val = it->second;
            }
            return true;
        }

        std::ostream& HttpRequest::dump(std::ostream& os) const //序列化输出到流中
        {
            //GET /uri?参数#片段 HTTP/1.1
            //Host: wwww.sylar.top
            //请求消息体 (请求数据)
            //
            os  << HttpMethodToString(m_method) << " "
                << m_path
                << (m_query.empty() ? "" : "?")
                << m_query
                << (m_fragment.empty() ? "" : "#")
                << m_fragment
                << " HTTP/"
                << ((uint32_t)(m_version >> 4))
                << "."
                << ((uint32_t)(m_version & 0x0F))   //0x0F  00001111
                << "\r\n";
            if(!m_websocket)
            {
                os << "connection: " << (m_close ? "close" : "keep-alive") << "\r\n";
            }
            for(auto& i : m_headers)    //Host: wwww.sylar.top
            {
                if(!m_websocket && strcasecmp(i.first.c_str(), "connection") == 0)  //strcasecmp判断字符串是否相等的函数，忽略大小写
                {
                    continue;
                }
                os << i.first << ": " << i.second << "\r\n";
            }

            if(!m_body.empty()) //请求消息体 (请求数据)
            {
                os << "content-length: " << m_body.size() << "\r\n\r\n"
                << m_body;
            }
            else
            {
                os << "\r\n";
            }
            return os;
        }

        std::string HttpRequest::toString() const   //转成字符串类型
        {
            std::stringstream ss;
            dump(ss);
            return ss.str();
        }

        // void HttpRequest::init()
        // {
        //     std::string conn = getHeader("connection");
        //     if(!conn.empty())
        //     {
        //         if(strcasecmp(conn.c_str(), "keep-alive") == 0)
        //         {
        //             m_close = false;
        //         }
        //         else
        //         {
        //             m_close = true;
        //         }
        //     }
        // }   

        // void HttpRequest::initParam()
        // {
        //     initQueryParam();
        //     initBodyParam();
        //     initCookies();
        // }

        // void HttpRequest::initQueryParam()
        // {
        //     if(m_parserParamFlag & 0x1)
        //     {
        //         return;
        //     }
        // }

        // #define PARSE_PARAM(str, m, flag, trim) \
        //     size_t pos = 0; \
        //     do { \
        //         size_t last = pos; \
        //         pos = str.find('=', pos); \
        //         if(pos == std::string::npos) { \
        //             break; \
        //         } \
        //         size_t key = pos; \
        //         pos = str.find(flag, pos); \
        //         m.insert(std::make_pair(trim(str.substr(last, key - last)), \
        //                     sylar::StringUtil::UrlDecode(str.substr(key + 1, pos - key - 1)))); \
        //         if(pos == std::string::npos) { \
        //             break; \
        //         } \
        //         ++pos; \
        //     } while(true);

        //     PARSE_PARAM(m_query, m_params, '&',);
        //     m_parserParamFlag |= 0x1;
        // }

        // void HttpRequest::initBodyParam()
        // {
        //     if(m_parserParamFlag & 0x2)
        //     {
        //         return;
        //     }
        //     std::string content_type = getHeader("content-type");
        //     if(strcasestr(content_type.c_str(), "application/x-www-form-urlencoded") == nullptr)
        //     {
        //         m_parserParamFlag |= 0x2;
        //         return;
        //     }
        //     PARSE_PARAM(m_body, m_params, '&',);
        //     m_parserParamFlag |= 0x2;
        // }

        // void HttpRequest::initCookies()
        // {
        //     if(m_parserParamFlag & 0x4)
        //     {
        //         return;
        //     }
        //     std::string cookie = getHeader("cookie");
        //     if(cookie.empty())
        //     {
        //         m_parserParamFlag |= 0x4;
        //         return;
        //     }
        //     PARSE_PARAM(cookie, m_cookies, ';', sylar::StringUtil::Trim);
        //     m_parserParamFlag |= 0x4;
        // }

        //HttpResponse

        HttpResponse::HttpResponse(uint8_t version, bool close) //构造函数  初始化
            :m_status(HttpStatus::OK)
            ,m_version(version)
            ,m_close(close)
            ,m_websocket(false)
        {}

        std::string HttpResponse::getHeader(const std::string& key, const std::string& def) const   //获取响应头部参数
        {
            auto it = m_headers.find(key);
            return it == m_headers.end() ? def : it->second;
        }

        void HttpResponse::setHeader(const std::string& key, const std::string& val)    //设置响应头部参数
        {
            m_headers[key] = val;
        }

        void HttpResponse::delHeader(const std::string& key)    //删除响应头部参数
        {
            m_headers.erase(key);
        }

        std::ostream& HttpResponse::dump(std::ostream& os) const    //序列化输出到流
        {
            //  HTTP/1.0 200 OK\r\n
            //  请求头部：参数
            //  Set-Cookie: 存储在用户主机中的文本文件
            //  ...
            //  响应消息体
            //  \r\n
            os  << "HTTP/"
                << ((uint32_t)(m_version >> 4))
                << "."
                << ((uint32_t)(m_version & 0x0F))
                << " "
                << (uint32_t)m_status
                << " "
                << (m_reason.empty() ? HttpStatusToString(m_status) : m_reason)
                << "\r\n";

            for(auto& i : m_headers)
            {
                if(!m_websocket && strcasecmp(i.first.c_str(), "connection") == 0)
                {
                    continue;
                }
                os << i.first << ": " << i.second << "\r\n";
            }
            for(auto& i : m_cookies)
            {
                os << "Set-Cookie: " << i << "\r\n";
            }
            if(!m_websocket)
            {
                os << "connection: " << (m_close ? "close" : "keep-alive") << "\r\n";
            }
            if(!m_body.empty())
            {
                os << "content-length: " << m_body.size() << "\r\n\r\n"
                << m_body;
            }
            else
            {
                os << "\r\n";
            }
            return os;
        }

        std::string HttpResponse::toString() const  //转成字符串
        {
            std::stringstream ss;
            dump(ss);
            return ss.str();
        }

        void HttpResponse::setRedirect(const std::string& uri)  //设置响应头部参数
        {
            m_status = HttpStatus::FOUND;
            setHeader("Location", uri);
        }

        // void HttpResponse::setCookie(const std::string& key, const std::string& val,
        //             time_t expired, const std::string& path,
        //             const std::string& domain, bool secure)
        // {
        //     std::stringstream ss;
        //     ss << key << "=" << val;
        //     if(expired > 0)
        //     {
        //         ss << ";expires=" << sylar::Time2Str(expired, "%a, %d %b %Y %H:%M:%S") << " GMT";
        //     }
        //     if(!domain.empty())
        //     {
        //         ss << ";domain=" << domain;
        //     }
        //     if(!path.empty())
        //     {
        //         ss << ";path=" << path;
        //     }
        //     if(secure)
        //     {
        //         ss << ";secure";
        //     }
        //     m_cookies.push_back(ss.str());
        // }

        //全局

        std::ostream& operator<<(std::ostream& os, const HttpRequest& req)  //流式输出HttpRequest
        {
            return req.dump(os);
        }

        std::ostream& operator<<(std::ostream& os, const HttpResponse& rsp) //流式输出HttpResponse
        {
            return rsp.dump(os);
        }

    }
}