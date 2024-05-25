#ifndef __SYLAR_HTTP_SERVLET_PATH_H__
#define __SYLAR_HTTP_SERVLET_PATH_H__
#include<memory>
#include"../config.h"
#include"servlet.h"

namespace sylar
{

    /**
     *  servlet请求路径配置结构体
    */
    struct ServletPathConf
    {
        typedef std::shared_ptr<ServletPathConf> ptr;
        
        std::string uri=""; //servlet请求文件的路径参数
        std::string type="html";    //servlet请求的文件类型

        bool operator==(const ServletPathConf& oth) const
        {
            return uri==oth.uri
                ,type==oth.type;
        }
    };

    /**
     *  类型转换模板类片特化(YAML String 转换成 ServletPathConf)
     *      (把请求路径文件配置文件 转化成 ServletPathConf结构体)
     */
    template<>
    class LexicalCast<std::string, ServletPathConf>
    {
    public:
        ServletPathConf operator()(const std::string& v)
        {
            YAML::Node node = YAML::Load(v);
            ServletPathConf conf;
            conf.uri = node["uri"].as<std::string>(conf.uri);  //conf.id默认值
            conf.type=node["type"].as<std::string>(conf.type);
            return conf;
        }
    };

    /**
     *  类型转换模板类片特化(ServletPathConf 转换成 YAML String)
     *      (把 ServletPathConf结构体 转化成 请求路径文件配置文件)
     */
    template<>
    class LexicalCast<ServletPathConf, std::string>
    {
    public:
        std::string operator()(const ServletPathConf& conf)
        {
            YAML::Node node;
            node["uri"] = conf.uri;
            node["type"]=conf.type;
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    namespace http
    {
        
        /**
         *  Servlet请求路径基类
        */
        class ServletPath
        {
        public:
            typedef std::shared_ptr<ServletPath> ptr;

            /**
             *  初始化
            */
            ServletPath(ServletPathConf conf);

            ServletPathConf getConf() const { return m_conf; }  //获取servlet请求路径配置结构体

            void setConf(ServletPathConf v) { m_conf=v; } //设置servlet请求路径配置结构体

            /**
             *  定义servlet匹配的回调函数
             *      dispatch servlet请求
             * return: 
             *      是否定义成功
            */
            virtual bool fun(sylar::http::ServletDispatch::ptr& dispatch)=0;

        protected:

            ServletPathConf m_conf; //servlet请求路径配置结构体
        
        };

        /**
         *  Servlet请求html路径
        */
        class ServletPathHtml : public ServletPath
        {
        public:
            typedef std::shared_ptr<ServletPathHtml> ptr;
            
            ServletPathHtml(ServletPathConf conf); //初始化

            /**
             *  定义servlet匹配的回调函数
             *      dispatch servlet请求
             * return: 
             *      是否定义成功
            */
            virtual bool fun(sylar::http::ServletDispatch::ptr& dispatch) override;

        };

    }
}

#endif