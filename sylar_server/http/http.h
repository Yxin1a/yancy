//      http协议封装
//#define XX(code, name, desc) name = code      返回值是name name=code序号
//HttpMethod    请求名称枚举name  name名称=num序号
//HttpStatus    请求名称枚举name  name名称=code序号
#ifndef __SYLAR_HTTP_HTTP_H__
#define __SYLAR_HTTP_HTTP_H__

#include<memory>
#include<string>
#include<map>
#include<vector>
#include<iostream>
#include<sstream>
#include<boost/lexical_cast.hpp>    //类型转换

namespace sylar
{
    namespace http
    {

//HTTP请求
/* Request Methods (HTTP方法枚举)*/
//  序号  名称   描述       ————————名称===描述)
#define HTTP_METHOD_MAP(XX)         \
  XX(0,  DELETE,      DELETE)       \
  XX(1,  GET,         GET)          \
  XX(2,  HEAD,        HEAD)         \
  XX(3,  POST,        POST)         \
  XX(4,  PUT,         PUT)          \
  /* pathological */                \
  XX(5,  CONNECT,     CONNECT)      \
  XX(6,  OPTIONS,     OPTIONS)      \
  XX(7,  TRACE,       TRACE)        \
  /* WebDAV */                      \
  XX(8,  COPY,        COPY)         \
  XX(9,  LOCK,        LOCK)         \
  XX(10, MKCOL,       MKCOL)        \
  XX(11, MOVE,        MOVE)         \
  XX(12, PROPFIND,    PROPFIND)     \
  XX(13, PROPPATCH,   PROPPATCH)    \
  XX(14, SEARCH,      SEARCH)       \
  XX(15, UNLOCK,      UNLOCK)       \
  XX(16, BIND,        BIND)         \
  XX(17, REBIND,      REBIND)       \
  XX(18, UNBIND,      UNBIND)       \
  XX(19, ACL,         ACL)          \
  /* subversion */                  \
  XX(20, REPORT,      REPORT)       \
  XX(21, MKACTIVITY,  MKACTIVITY)   \
  XX(22, CHECKOUT,    CHECKOUT)     \
  XX(23, MERGE,       MERGE)        \
  /* upnp */                        \
  XX(24, MSEARCH,     M-SEARCH)     \
  XX(25, NOTIFY,      NOTIFY)       \
  XX(26, SUBSCRIBE,   SUBSCRIBE)    \
  XX(27, UNSUBSCRIBE, UNSUBSCRIBE)  \
  /* RFC-5789 */                    \
  XX(28, PATCH,       PATCH)        \
  XX(29, PURGE,       PURGE)        \
  /* CalDAV */                      \
  XX(30, MKCALENDAR,  MKCALENDAR)   \
  /* RFC-2068, section 19.6.1.2 */  \
  XX(31, LINK,        LINK)         \
  XX(32, UNLINK,      UNLINK)       \
  /* icecast */                     \
  XX(33, SOURCE,      SOURCE)       \

//HTTP响应
/* Status Codes (HTTP状态枚举)*/
//  序号  名称   描述       ————————名称===描述)
#define HTTP_STATUS_MAP(XX)                                                 \
  XX(100, CONTINUE,                        Continue)                        \
  XX(101, SWITCHING_PROTOCOLS,             Switching Protocols)             \
  XX(102, PROCESSING,                      Processing)                      \
  XX(200, OK,                              OK)                              \
  XX(201, CREATED,                         Created)                         \
  XX(202, ACCEPTED,                        Accepted)                        \
  XX(203, NON_AUTHORITATIVE_INFORMATION,   Non-Authoritative Information)   \
  XX(204, NO_CONTENT,                      No Content)                      \
  XX(205, RESET_CONTENT,                   Reset Content)                   \
  XX(206, PARTIAL_CONTENT,                 Partial Content)                 \
  XX(207, MULTI_STATUS,                    Multi-Status)                    \
  XX(208, ALREADY_REPORTED,                Already Reported)                \
  XX(226, IM_USED,                         IM Used)                         \
  XX(300, MULTIPLE_CHOICES,                Multiple Choices)                \
  XX(301, MOVED_PERMANENTLY,               Moved Permanently)               \
  XX(302, FOUND,                           Found)                           \
  XX(303, SEE_OTHER,                       See Other)                       \
  XX(304, NOT_MODIFIED,                    Not Modified)                    \
  XX(305, USE_PROXY,                       Use Proxy)                       \
  XX(307, TEMPORARY_REDIRECT,              Temporary Redirect)              \
  XX(308, PERMANENT_REDIRECT,              Permanent Redirect)              \
  XX(400, BAD_REQUEST,                     Bad Request)                     \
  XX(401, UNAUTHORIZED,                    Unauthorized)                    \
  XX(402, PAYMENT_REQUIRED,                Payment Required)                \
  XX(403, FORBIDDEN,                       Forbidden)                       \
  XX(404, NOT_FOUND,                       Not Found)                       \
  XX(405, METHOD_NOT_ALLOWED,              Method Not Allowed)              \
  XX(406, NOT_ACCEPTABLE,                  Not Acceptable)                  \
  XX(407, PROXY_AUTHENTICATION_REQUIRED,   Proxy Authentication Required)   \
  XX(408, REQUEST_TIMEOUT,                 Request Timeout)                 \
  XX(409, CONFLICT,                        Conflict)                        \
  XX(410, GONE,                            Gone)                            \
  XX(411, LENGTH_REQUIRED,                 Length Required)                 \
  XX(412, PRECONDITION_FAILED,             Precondition Failed)             \
  XX(413, PAYLOAD_TOO_LARGE,               Payload Too Large)               \
  XX(414, URI_TOO_LONG,                    URI Too Long)                    \
  XX(415, UNSUPPORTED_MEDIA_TYPE,          Unsupported Media Type)          \
  XX(416, RANGE_NOT_SATISFIABLE,           Range Not Satisfiable)           \
  XX(417, EXPECTATION_FAILED,              Expectation Failed)              \
  XX(421, MISDIRECTED_REQUEST,             Misdirected Request)             \
  XX(422, UNPROCESSABLE_ENTITY,            Unprocessable Entity)            \
  XX(423, LOCKED,                          Locked)                          \
  XX(424, FAILED_DEPENDENCY,               Failed Dependency)               \
  XX(426, UPGRADE_REQUIRED,                Upgrade Required)                \
  XX(428, PRECONDITION_REQUIRED,           Precondition Required)           \
  XX(429, TOO_MANY_REQUESTS,               Too Many Requests)               \
  XX(431, REQUEST_HEADER_FIELDS_TOO_LARGE, Request Header Fields Too Large) \
  XX(451, UNAVAILABLE_FOR_LEGAL_REASONS,   Unavailable For Legal Reasons)   \
  XX(500, INTERNAL_SERVER_ERROR,           Internal Server Error)           \
  XX(501, NOT_IMPLEMENTED,                 Not Implemented)                 \
  XX(502, BAD_GATEWAY,                     Bad Gateway)                     \
  XX(503, SERVICE_UNAVAILABLE,             Service Unavailable)             \
  XX(504, GATEWAY_TIMEOUT,                 Gateway Timeout)                 \
  XX(505, HTTP_VERSION_NOT_SUPPORTED,      HTTP Version Not Supported)      \
  XX(506, VARIANT_ALSO_NEGOTIATES,         Variant Also Negotiates)         \
  XX(507, INSUFFICIENT_STORAGE,            Insufficient Storage)            \
  XX(508, LOOP_DETECTED,                   Loop Detected)                   \
  XX(510, NOT_EXTENDED,                    Not Extended)                    \
  XX(511, NETWORK_AUTHENTICATION_REQUIRED, Network Authentication Required) \

/**
 *  HTTP请求方法枚举类(枚举类解决同个作用域内，枚举类型的成员不能重名的问题。)
 *      HttpMethod 是名称枚举(name=num序号  GET=1)
 */
enum class HttpMethod
{
#define XX(num, name, string) name = num,
    HTTP_METHOD_MAP(XX)
#undef XX
    INVALID_METHOD  //不存在(没有)
};

/**
 *  HTTP响应状态枚举类(枚举类解决同个作用域内，枚举类型的成员不能重名的问题。)
 *      HttpStatus 是名称枚举(name=code序号  GET=1)
 */
enum class HttpStatus
{
#define XX(code, name, desc) name = code,
    HTTP_STATUS_MAP(XX)
#undef XX
};

/**
 *  将字符串方法名转成HTTP请求方法枚举
 *      m HTTP方法
 * return:
 *      HTTP请求方法枚举
 */
HttpMethod StringToHttpMethod(const std::string& m);

/**
 *  将字符串指针转换成HTTP请求方法枚举
 *      m 字符串方法枚举
 * return:
 *      HTTP请求方法枚举
 */
HttpMethod CharsToHttpMethod(const char* m);

/**
 *  将HTTP方法枚举转换成字符串
 *      m HTTP请求方法枚举
 * return:
 *      字符串
 */
const char* HttpMethodToString(const HttpMethod& m);

/**
 *  将HTTP响应状态枚举转换成字符串
 *      m HTTP响应状态枚举
 * return:
 *      字符串
 */
const char* HttpStatusToString(const HttpStatus& s);

/**
 *  忽略大小写比较是否相等仿函数
 */
struct CaseInsensitiveLess
{
    /**
     *  忽略大小写比较字符串是否相等
     */
    bool operator()(const std::string& lhs, const std::string& rhs) const;
};

/**
 *  获取Map中的key值,并转成对应类型,返回是否成功
 *      m Map数据结构
 *      key 关键字
 *      val 保存转换后的值
 *      def 默认值
 * return:
 *      true 转换成功, val 为对应的值
 *      false 不存在或者转换失败 val = def
 */
template<class MapType, class T>
bool checkGetAs(const MapType& m, const std::string& key, T& val, const T& def = T())
{
    auto it = m.find(key);  //寻找
    if(it == m.end())   //不存在
    {
        val = def;
        return false;
    }
    try
    {
        val = boost::lexical_cast<T>(it->second);   //lexical_cast<>函数将 参数 转化成 对应的类型
        return true;
    }
    catch (...)
    {
        val = def;
    }
    return false;
}

/**
 *  获取Map中的key值,并转成对应类型
 *      m Map数据结构
 *      key 关键字
 *      def 默认值
 * return:
 *      如果存在且转换成功返回对应的值,否则返回默认值
 */
template<class MapType, class T>
T getAs(const MapType& m, const std::string& key, const T& def = T())
{
    auto it = m.find(key);  //寻找
    if(it == m.end())   //不存在
    {
        return def;
    }
    try
    {
        return boost::lexical_cast<T>(it->second);  //lexical_cast<>函数将 参数 转化成 对应的类型
    }
    catch (...)
    {}
    return def;
}


        class HttpResponse;

        /**
         *  HTTP请求结构体
         */
        class HttpRequest
        {
        public:
            typedef std::shared_ptr<HttpRequest> ptr;   //HTTP请求的智能指针
            typedef std::map<std::string, std::string, CaseInsensitiveLess> MapType;    //MAP结构(key  请求协议的参数)
                                                                                        //       (Host: wwww.sylar.top)
                                                                                        //       key=Host  请求协议的参数=wwww.sylar.top
                                                                                        //CaseInsensitiveLess忽略大小写比较是否相等仿函数
            
            /**
             *  构造函数
             *      version 版本(0x11 11)
             *      close 是否keepalive(自动关闭)
             */
            HttpRequest(uint8_t version = 0x11, bool close = true);

            std::shared_ptr<HttpResponse> createResponse();

            HttpMethod getMethod() const { return m_method;}        //返回HTTP请求方法(名称=序号  GET=1)
            uint8_t getVersion() const { return m_version;}         //返回HTTP版本
            const std::string& getPath() const { return m_path;}    //返回HTTP请求的路径
            const std::string& getQuery() const { return m_query;}  //返回HTTP请求的查询参数
            const std::string& getBody() const { return m_body;}    //返回HTTP请求的消息体
            const MapType& getHeaders() const { return m_headers;}  //返回HTTP请求的消息头MAP   (Host: wwww.sylar.top)
            const MapType& getParams() const { return m_params;}    //返回HTTP请求的参数MAP
            const MapType& getCookies() const { return m_cookies;}  //返回HTTP请求的cookie MAP

            void setMethod(HttpMethod v) { m_method = v;}   //设置HTTP请求的方法(名称=序号  GET=1)
            void setVersion(uint8_t v) { m_version = v;}    //设置HTTP请求的协议版本
            void setPath(const std::string& v) { m_path = v;}   //设置HTTP请求的路径
            void setQuery(const std::string& v) { m_query = v;} //设置HTTP请求的查询参数
            void setFragment(const std::string& v) { m_fragment = v;}   //设置HTTP请求的Fragment(片段)
            void setBody(const std::string& v) { m_body = v;}   //设置HTTP请求的消息体

            bool isClose() const { return m_close;} //是否自动关闭
            void setClose(bool v) { m_close = v;}   //设置是否自动关闭
            bool isWebsocket() const { return m_websocket;} //是否websocket(网络套接字)
            void setWebsocket(bool v) { m_websocket = v;}   //设置是否websocket(网络套接字)

            void setHeaders(const MapType& v) { m_headers = v;} //设置HTTP请求的头部MAP
            void setParams(const MapType& v) { m_params = v;}   //设置HTTP请求的参数MAP
            void setCookies(const MapType& v) { m_cookies = v;} //设置HTTP请求的Cookie MAP

            /**
             *  获取HTTP请求的头部参数
             *      key 关键字
             *      def 默认值
             * return:
             *      如果存在则返回对应值,否则返回默认值
             */
            std::string getHeader(const std::string& key, const std::string& def = "") const;

            /**
             *  获取HTTP请求的请求参数
             *      key 关键字
             *      def 默认值
             * return:
             *      如果存在则返回对应值,否则返回默认值
             */
            std::string getParam(const std::string& key, const std::string& def = "");

            /**
             *  获取HTTP请求的Cookie参数
             *      key 关键字
             *      def 默认值
             * return:
             *      如果存在则返回对应值,否则返回默认值
             */
            std::string getCookie(const std::string& key, const std::string& def = "");

            
            /**
             *  设置HTTP请求的头部参数
             *      key 关键字
             *      val 参数
             */
            void setHeader(const std::string& key, const std::string& val);

            /**
             *  设置HTTP请求的请求参数
             *      key 关键字
             *      val 参数
             */

            void setParam(const std::string& key, const std::string& val);
            /**
             *  设置HTTP请求的Cookie参数
             *      key 关键字
             *      val 参数
             */
            void setCookie(const std::string& key, const std::string& val);

            /**
             *  删除HTTP请求的头部参数
             *      key 关键字
             */
            void delHeader(const std::string& key);

            /**
             *  删除HTTP请求的请求参数
             *      key 关键字
             */
            void delParam(const std::string& key);

            /**
             *  删除HTTP请求的Cookie参数
             *      key 关键字
             */
            void delCookie(const std::string& key);

            /**
             *  判断HTTP请求的头部参数是否存在
             *      key 关键字
             *      val 如果存在,val非空则赋值(val=参数)
             * return:
             *      是否存在
             */
            bool hasHeader(const std::string& key, std::string* val = nullptr);

            /**
             *  判断HTTP请求的请求参数是否存在
             *      key 关键字
             *      val 如果存在,val非空则赋值(val=参数)
             * return:
             *      是否存在
             */
            bool hasParam(const std::string& key, std::string* val = nullptr);

            /**
             *  判断HTTP请求的Cookie参数是否存在
             *      key 关键字
             *      val 如果存在,val非空则赋值(val=参数)
             * return:
             *      是否存在
             */
            bool hasCookie(const std::string& key, std::string* val = nullptr);

            /**
             *  检查并获取HTTP请求的头部参数
             *      T 转换类型
             *      key 关键字
             *      val 返回值(val=参数)
             *      def 默认值
             * return:
             *      如果存在且转换成功返回true,否则失败val=def
             */
            template<class T>
            bool checkGetHeaderAs(const std::string& key, T& val, const T& def = T())
            {
                return checkGetAs(m_headers, key, val, def);    //获取Map中的key值,并转成对应类型,返回是否成功
            }

            /**
             *  获取HTTP请求的头部参数
             *      T 转换类型
             *      key 关键字
             *      def 默认值
             * return:
             *      如果存在且转换成功返回对应的值,否则返回def
             */
            template<class T>
            T getHeaderAs(const std::string& key, const T& def = T())
            {
                return getAs(m_headers, key, def);  //获取Map中的key值,并转成对应类型
            }

            /**
             *  检查并获取HTTP请求的请求参数
             *      T 转换类型
             *      key 关键字
             *      val 返回值(val=参数)
             *      def 默认值
             * return:
             *      如果存在且转换成功返回true,否则失败val=def
             */
            template<class T>
            bool checkGetParamAs(const std::string& key, T& val, const T& def = T())
            {
                //initQueryParam();
                //initBodyParam();
                return checkGetAs(m_params, key, val, def); //获取Map中的key值,并转成对应类型,返回是否成功
            }

            /**
             *  获取HTTP请求的请求参数
             *      T 转换类型
             *      key 关键字
             *      def 默认值
             * return:
             *      如果存在且转换成功返回对应的值,否则返回def
             */
            template<class T>
            T getParamAs(const std::string& key, const T& def = T())
            {
                //initQueryParam();
                //initBodyParam();
                return getAs(m_params, key, def);   //获取Map中的key值,并转成对应类型
            }

            /**
             *  检查并获取HTTP请求的Cookie参数
             *      T 转换类型
             *      key 关键字
             *      val 返回值(val=参数)
             *      def 默认值
             * return:
             *      如果存在且转换成功返回true,否则失败val=def
             */
            template<class T>
            bool checkGetCookieAs(const std::string& key, T& val, const T& def = T())
            {
                //initCookies();
                return checkGetAs(m_cookies, key, val, def);    //获取Map中的key值,并转成对应类型,返回是否成功
            }

            /**
             *  获取HTTP请求的Cookie参数
             *      T 转换类型
             *      key 关键字
             *      def 默认值
             * return:
             *      如果存在且转换成功返回对应的值,否则返回def
             */
            template<class T>
            T getCookieAs(const std::string& key, const T& def = T())
            {
                //initCookies();
                return getAs(m_cookies, key, def);  //获取Map中的key值,并转成对应类型
            }

            std::ostream& dump(std::ostream& os) const; //序列化输出到流中
            std::string toString() const;   //转成字符串类型

            // void init();
            // void initParam();
            // void initQueryParam();
            // void initBodyParam();
            // void initCookies();
            
        private:
            /**
             * GET /sample.jsp HTTP/1.1(请求方法 URL 版本)
             * Accept-Encoding: gzip, deflate(请求头部)
             * ...
             * 空行
             * 请求正文(请求头和请求正文之间是一个空行，这个行非常重要)
            */

            HttpMethod m_method;    //HTTP请求方法(名称=序号  GET=1)
            uint8_t m_version;  //HTTP版本
            bool m_close;   //是否自动关闭
            bool m_websocket;   //是否为websocket(网络套接字)

            // URL: ( 协议类型://服务器地址[:端口号]/路径/文件名[参数=值] )
            ///     路径/文件名[参数=值]
            //      m_path?m_query#m_fragment
            uint8_t m_parserParamFlag;  //
            std::string m_path; //请求对应的路径
            std::string m_query;    //请求参数
            std::string m_fragment; //请求fragment

            std::string m_body; //请求消息体    (请求正文)
            MapType m_headers;  //请求头部MAP   (Host: wwww.sylar.top)
            MapType m_params;   //请求参数MAP
            MapType m_cookies;  //请求Cookie MAP(存储在用户主机中的文本文件)
        };
    
        /**
         *  HTTP响应结构体
         */
        class HttpResponse
        {
        public:
            typedef std::shared_ptr<HttpResponse> ptr;  //HTTP响应结构智能指针
            typedef std::map<std::string, std::string, CaseInsensitiveLess> MapType;    //MAP结构(key:  响应头部参数)
                                                                                        //       (Host: wwww.sylar.top)
                                                                                        //       key=Host  响应头部参数=wwww.sylar.top
                                                                                        //CaseInsensitiveLess忽略大小写比较是否相等仿函数
            
            /**
             *  构造函数
             *      version 版本
             *      close 是否自动关闭
             */
            HttpResponse(uint8_t version = 0x11, bool close = true);

            HttpStatus getStatus() const { return m_status;}    //返回响应状态

            uint8_t getVersion() const { return m_version;} //返回响应版本

            const std::string& getBody() const { return m_body;}        //返回响应消息体
            const std::string& getReason() const { return m_reason;}    //返回响应原因(相当于响应的状态)
            const MapType& getHeaders() const { return m_headers;}      //返回响应头部MAP
            void setStatus(HttpStatus v) { m_status = v;}               //设置响应状态
            void setVersion(uint8_t v) { m_version = v;}                //设置响应版本
            void setBody(const std::string& v) { m_body = v;}           //设置响应消息体
            void setReason(const std::string& v) { m_reason = v;}       //设置响应原因(相当于响应的状态)
            void setHeaders(const MapType& v) { m_headers = v;}         //设置响应头部MAP
            bool isClose() const { return m_close;}                     //返回是否自动关闭
            void setClose(bool v) { m_close = v;}                       //设置是否自动关闭
            bool isWebsocket() const { return m_websocket;}             //返回是否websocket(网络套接字)
            void setWebsocket(bool v) { m_websocket = v;}               //设置是否websocket(网络套接字)

            /**
             *  获取响应头部参数
             *      key 关键字
             *      def 默认值
             * return:
             *      如果存在返回对应值,否则返回def
             */
            std::string getHeader(const std::string& key, const std::string& def = "") const;

            /**
             *  设置响应头部参数
             *      key 关键字
             *      val 值
             */
            void setHeader(const std::string& key, const std::string& val);

            /**
             *  删除响应头部参数
             *      key 关键字
             */
            void delHeader(const std::string& key);

            /**
             *  检查并获取响应头部参数
             *      T 值类型
             *      key 关键字
             *      val 值
             *      def 默认值
             * return:如果存在且转换成功返回true,否则失败val=def
             */
            template<class T>
            bool checkGetHeaderAs(const std::string& key, T& val, const T& def = T())
            {
                return checkGetAs(m_headers, key, val, def);
            }

            /**
             *  获取响应的头部参数
             *      T 转换类型
             *      key 关键字
             *      def 默认值
             * return:如果存在且转换成功返回对应的值,否则返回def
             */
            template<class T>
            T getHeaderAs(const std::string& key, const T& def = T())
            {
                return getAs(m_headers, key, def);
            }

            /**
             *  序列化输出到流
             *      os 输出流
             * return:
             *      输出流
             */
            std::ostream& dump(std::ostream& os) const;

            /**
             *  转成字符串
             */
            std::string toString() const;

            void setRedirect(const std::string& uri);   //设置响应头部参数
            void setCookie(const std::string& key, const std::string& val,  //设置响应Cookie vector(存储在用户主机中的文本文件)
                        time_t expired = 0, const std::string& path = "",
                        const std::string& domain = "", bool secure = false);

        private:
            //HTTP/1.1 200 OK(版本 状态码 状态信息——对状态码的描述)
            //Content-Type: text/html(响应头部)
            //...
            //空行
            //响应正文
            
            HttpStatus m_status;    //响应状态
            uint8_t m_version;  //版本
            bool m_close;   //是否自动关闭
            bool m_websocket;   //是否为websocket(网络套接字)

            std::string m_body; //响应消息体(响应正文)
            std::string m_reason;   //响应原因(相当于响应的状态)
            MapType m_headers;  //响应头部MAP

            std::vector<std::string> m_cookies; //响应Cookie vector(存储在用户主机中的文本文件)
        };

        /**
         *  流式输出HttpRequest
         *      os 输出流
         *      req HTTP请求
         * return:
         *      输出流
         */
        std::ostream& operator<<(std::ostream& os, const HttpRequest& req);

        /**
         *  流式输出HttpResponse
         *      os 输出流
         *      rsp HTTP响应
         * return:
         *      输出流
         */
        std::ostream& operator<<(std::ostream& os, const HttpResponse& rsp);

    }

}

#endif