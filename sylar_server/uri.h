//  URI封装类
//解析URI网址(http://www.sylar.top/test/uri?id=100&name=sylar#frg),并判断是否是对的
/*
        scheme://userinfo@host:port path?query#fragment

        http://user@sylar.com:8042/over/there?name=ferret#nose
        \_/   \__________________/\_________/ \_________/ \__/
         |              |              |           |        |
        scheme      authority         path       query    fragment
*/
#ifndef __SYLAR_URI_H__
#define __SYLAR_URI_H__

#include <memory>
#include <string>
#include <stdint.h>
#include "address.h"

namespace sylar
{

    

    /**
     *  URI类
     *      (网址————域名)
     */
    class Uri
    {
    public:

        typedef std::shared_ptr<Uri> ptr;   //智能指针类型定义

        /**
         *  创建Uri对象，并解析uri(网址)
         *      uri uri字符串
         * return: 解析成功返回Uri对象否则返回nullptr
         */
        static Uri::ptr Create(const std::string& uri);

        /**
         *  构造函数
         */
        Uri();

        const std::string& getScheme() const { return m_scheme;}    //返回scheme
        const std::string& getUserinfo() const { return m_userinfo;}    //返回用户信息
        const std::string& getHost() const { return m_host;}    //返回host
        const std::string& getPath() const; //返回路径
        const std::string& getQuery() const { return m_query;}  //返回查询条件
        const std::string& getFragment() const { return m_fragment;}    //返回fragment
        int32_t getPort() const;    //返回端口

        void setScheme(const std::string& v) { m_scheme = v;}   //设置scheme
        void setUserinfo(const std::string& v) { m_userinfo = v;}   //设置用户信息
        void setHost(const std::string& v) { m_host = v;}   //设置host信息
        void setPath(const std::string& v) { m_path = v;}   //设置路径
        void setQuery(const std::string& v) { m_query = v;} //设置查询条件
        void setFragment(const std::string& v) { m_fragment = v;}   //设置fragment
        void setPort(int32_t v) { m_port = v;}  //设置端口号

        std::ostream& dump(std::ostream& os) const; //序列化到输出流

        std::string toString() const;   //转成字符串

        Address::ptr createAddress() const; //根据uri类，获取Address

    private:

        bool isDefaultPort() const; //是否默认端口(0、http 80、https 443)

    private:
        /*
                scheme://userinfo@host:port path?query#fragment

                http://user@sylar.com:8042/over/there?name=ferret#nose
                \_/   \__________________/\_________/ \_________/ \__/
                |              |              |           |        |
                scheme      authority         path       query    fragment
        */
        std::string m_scheme;   //schema
        std::string m_userinfo; //用户信息
        std::string m_host; //host
        std::string m_path; //路径
        std::string m_query;    //查询参数
        std::string m_fragment; //fragment
        int32_t m_port; //端口
    };

}

#endif
