//  Servlet封装(请求http服务器中的html等文件)
//(通过路径来匹配服务器已存储Servlet(路径)，①匹配到了返回Servlet相对应的响应协议消息体(正文)————服务器中的html等文件，
//                                  ②匹配不到返回404 NotFoundServlet，访问错误)
//(虚拟接口)
#ifndef __SYLAR_HTTP_SERVLET_H__
#define __SYLAR_HTTP_SERVLET_H__

#include <memory>
#include <functional>
#include <string>
#include <vector>
#include <unordered_map>
#include "http.h"
#include "http_session.h"
#include "sylar_server/thread.h"
#include "sylar_server/util.h"

namespace sylar
{

    namespace http
    {

        /**
         *  Servlet封装
         */
        class Servlet
        {
        public:

            typedef std::shared_ptr<Servlet> ptr;   //智能指针类型定义

            /**
             *  构造函数(初始化)
             *      name Servlet名称
             */
            Servlet(const std::string& name)
                :m_name(name) {}

            /**
             *  析构函数
             */
            virtual ~Servlet() {}

            /**
             *  处理请求
             *      request HTTP请求
             *      response HTTP响应
             *      session HTTP连接
             * return: 
             *      是否处理成功
             */
            virtual int32_t handle(sylar::http::HttpRequest::ptr request
                        , sylar::http::HttpResponse::ptr response
                        , sylar::http::HttpSession::ptr session) = 0;

            const std::string& getName() const { return m_name;}    //返回Servlet名称

        protected:

            std::string m_name; //Servlet名称

        };

        /**
         *  回调函数式Servlet
         *      (可通过该类对象来使用、添加Servlet类)
         */
        class FunctionServlet : public Servlet
        {
        public:

            typedef std::shared_ptr<FunctionServlet> ptr;   //智能指针类型定义

            typedef std::function<int32_t (sylar::http::HttpRequest::ptr request
                        , sylar::http::HttpResponse::ptr response
                        , sylar::http::HttpSession::ptr session)> callback; //函数回调类型定义  Servlet签名


            /**
             *  构造函数(初始化)
             *      cb 回调函数
             */
            FunctionServlet(callback cb);

            /**
             *  处理请求
             *      request HTTP请求
             *      response HTTP响应
             *      session HTTP连接
             * return: 
             *      是否处理成功
             */
            virtual int32_t handle(sylar::http::HttpRequest::ptr request
                        , sylar::http::HttpResponse::ptr response
                        , sylar::http::HttpSession::ptr session) override;

        private:

            callback m_cb;  //回调函数

        };

        /**
         *  Servlet指针封装
        */
        class IServletCreator
        {
        public:
            typedef std::shared_ptr<IServletCreator> ptr;
            virtual ~IServletCreator() {}
            virtual Servlet::ptr get() const = 0;
            virtual std::string getName() const = 0;
        };

        class HoldServletCreator : public IServletCreator
        {
        public:
            typedef std::shared_ptr<HoldServletCreator> ptr;

            HoldServletCreator(Servlet::ptr slt)    //初始化servlet指针
                :m_servlet(slt) {}

            Servlet::ptr get() const override { return m_servlet; } //返回Servlet指针

            std::string getName() const override { return m_servlet->getName(); }   //返回servlet名称

        private:

            Servlet::ptr m_servlet; //Servlet指针

        };

        // template<class T>
        // class ServletCreator : public IServletCreator
        // {
        // public:
        //     typedef std::shared_ptr<ServletCreator> ptr;

        //     ServletCreator() {}

        //     Servlet::ptr get() const override { return Servlet::ptr(new T); }

        //     std::string getName() const override { return TypeToName<T>(); }

        // };

        /**
         *  Servlet分发器(管理类)
         *      匹配servlet
         */
        class ServletDispatch : public Servlet
        {
        public:

            typedef std::shared_ptr<ServletDispatch> ptr;   //智能指针类型定义

            typedef RWMutex RWMutexType;    //读写锁类型定义

            /**
             *  构造函数
             */
            ServletDispatch();

            /**
             *  处理请求(决定具体请求哪个Servlet)
             *      request HTTP请求
             *      response HTTP响应
             *      session HTTP连接
             * return: 
             *      是否处理成功
             */
            virtual int32_t handle(sylar::http::HttpRequest::ptr request
                        , sylar::http::HttpResponse::ptr response
                        , sylar::http::HttpSession::ptr session) override;

            /**
             *  添加精准匹配servlet(普通)
             *      uri uri路径
             *      slt serlvet
             */
            void addServlet(const std::string& uri, Servlet::ptr slt);

            /**
             *  添加精准匹配servlet(回调函数)
             *      uri uri路径
             *      cb FunctionServlet回调函数
             */
            void addServlet(const std::string& uri, FunctionServlet::callback cb);

            /**
             *  添加模糊匹配servlet(普通)
             *      uri uri路径 模糊匹配 /sylar_*
             *      slt servlet
             */
            void addGlobServlet(const std::string& uri, Servlet::ptr slt);

            /**
             *  添加模糊匹配servlet(回调函数)
             *      uri uri路径 模糊匹配 /sylar_*
             *      cb FunctionServlet回调函数
             */
            void addGlobServlet(const std::string& uri, FunctionServlet::callback cb);

            /**
             *  添加精准匹配servlet(Servlet指针封装)
             *      uri uri路径
             *      creator IServletCreator
             */
            void addServletCreator(const std::string& uri, IServletCreator::ptr creator);

            /**
             *  添加模糊匹配servlet(Servlet指针封装)
             *      uri uri路径 模糊匹配 /sylar_*
             *      cb IServletCreator
             */
            void addGlobServletCreator(const std::string& uri, IServletCreator::ptr creator);

            // template<class T>
            // void addServletCreator(const std::string& uri)
            // {
            //     addServletCreator(uri, std::make_shared<ServletCreator<T> >());
            // }

            // template<class T>
            // void addGlobServletCreator(const std::string& uri)
            // {
            //     addGlobServletCreator(uri, std::make_shared<ServletCreator<T> >());
            // }

            /**
             *  删除精准匹配servlet
             *      uri uri路径
             */
            void delServlet(const std::string& uri);

            /**
             *  删除模糊匹配servlet
             *      uri uri路径
             */
            void delGlobServlet(const std::string& uri);

            Servlet::ptr getDefault() const { return m_default;}    //返回默认servlet

            void setDefault(Servlet::ptr v) { m_default = v;}   //设置默认servlet


            /**
             *  通过uri获取精准匹配servlet
             *      uri uri路径
             * return: 
             *      返回对应的servlet
             */
            Servlet::ptr getServlet(const std::string& uri);

            /**
             *  通过uri获取模糊匹配servlet
             *      uri uri路径
             * return: 
             *      返回对应的servlet
             */
            Servlet::ptr getGlobServlet(const std::string& uri);

            /**
             *  通过uri获取servlet
             *      uri uri路径
             * return: 
             *      优先精准匹配,其次模糊匹配,最后返回默认
             */
            Servlet::ptr getMatchedServlet(const std::string& uri);

            void listAllServletCreator(std::map<std::string, IServletCreator::ptr>& infos); //复制精准匹配servlet MAP

            void listAllGlobServletCreator(std::map<std::string, IServletCreator::ptr>& infos); //复制模糊匹配servlet 数组

        private:

            RWMutexType m_mutex;    //读写互斥量

            /// 精准匹配servlet MAP
            /// uri(/sylar/xxx)路径 -> servlet
            std::unordered_map<std::string, IServletCreator::ptr> m_datas;

            /// 模糊匹配servlet 数组
            /// uri(/sylar/*)路径 -> servlet
            std::vector<std::pair<std::string, IServletCreator::ptr> > m_globs;

            Servlet::ptr m_default; //默认servlet，所有路径都没匹配到时使用

        };

        /**
         *  NotFoundServlet(默认Servlet)
         *      (默认返回404)
         *      (不存在，什么都用不了)
         */
        class NotFoundServlet : public Servlet
        {
        public:

            typedef std::shared_ptr<NotFoundServlet> ptr;   //智能指针类型定义

            /**
             *  构造函数
             */
            NotFoundServlet(const std::string& name);

            /**
             *  处理(访问错误)
             *      request HTTP请求
             *      response HTTP响应
             *      session HTTP连接
             * return: 
             *      是否处理成功
             */
            virtual int32_t handle(sylar::http::HttpRequest::ptr request
                        , sylar::http::HttpResponse::ptr response
                        , sylar::http::HttpSession::ptr session) override;

        private:

            std::string m_name;     //服务器名称
            std::string m_content;  //响应的内容
            
        };

    }
}

#endif
