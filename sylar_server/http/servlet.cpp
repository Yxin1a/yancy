#include "servlet.h"
#include <fnmatch.h>

namespace sylar
{
    namespace http
    {
        
        //FunctionServlet
        FunctionServlet::FunctionServlet(callback cb)   //初始化    回调函数初始化Servlet
            :Servlet("FunctionServlet")
            ,m_cb(cb) 
        {}

        int32_t FunctionServlet::handle(sylar::http::HttpRequest::ptr request   //处理请求(回调函数)
                    , sylar::http::HttpResponse::ptr response
                    , sylar::http::HttpSession::ptr session)
        {
            return m_cb(request, response, session);    //执行回调函数cb
        }

        //ServletDispatch
        ServletDispatch::ServletDispatch()  //初始化 初始化Servlet 默认Servlet
            :Servlet("ServletDispatch")
        {
            m_default.reset(new NotFoundServlet("sylar/1.0"));
        }

        int32_t ServletDispatch::handle(sylar::http::HttpRequest::ptr request   //处理请求(决定具体请求哪个Servlet)
                    , sylar::http::HttpResponse::ptr response
                    , sylar::http::HttpSession::ptr session)
        {
            auto slt = getMatchedServlet(request->getPath());
            if(slt)
            {
                slt->handle(request, response, session);
            }
            return 0;
        }

        void ServletDispatch::addServlet(const std::string& uri, Servlet::ptr slt)  //添加精准匹配servlet(普通)
        {
            RWMutexType::WriteLock lock(m_mutex);
            m_datas[uri] = std::make_shared<HoldServletCreator>(slt);
        }

        void ServletDispatch::addServletCreator(const std::string& uri, IServletCreator::ptr creator)   //添加精准匹配servlet(Servlet指针封装)
        {
            RWMutexType::WriteLock lock(m_mutex);
            m_datas[uri] = creator;
        }

        void ServletDispatch::addGlobServletCreator(const std::string& uri, IServletCreator::ptr creator)   //添加模糊匹配servlet(Servlet指针封装)
        {
            RWMutexType::WriteLock lock(m_mutex);
            for(auto it = m_globs.begin(); it != m_globs.end(); ++it)   ////更新，先删除后插入
            {
                if(it->first == uri)
                {
                    m_globs.erase(it);
                    break;
                }
            }
            m_globs.push_back(std::make_pair(uri, creator));
        }

        void ServletDispatch::addServlet(const std::string& uri //添加精准匹配servlet(回调函数)
                                ,FunctionServlet::callback cb)  //cb回调函数
        {
            RWMutexType::WriteLock lock(m_mutex);
            m_datas[uri] = std::make_shared<HoldServletCreator>(
                                std::make_shared<FunctionServlet>(cb)); //make_shared在动态内存中分配一个对象并初始化它，返回指向此对象的shared_ptr
        }

        void ServletDispatch::addGlobServlet(const std::string& uri,Servlet::ptr slt)   //添加模糊匹配servlet(普通)
        {
            RWMutexType::WriteLock lock(m_mutex);
            for(auto it = m_globs.begin();it != m_globs.end(); ++it)    //更新，先删除后插入
            {
                if(it->first == uri)
                {
                    m_globs.erase(it);
                    break;
                }
            }
            m_globs.push_back(std::make_pair(uri, std::make_shared<HoldServletCreator>(slt)));  //make_pair将两个数据组合成一个数据，两个数据可以是同一类型或者不同类型。
                                                                                                //make_shared在动态内存中分配一个对象并初始化它，返回指向此对象的shared_ptr
        }

        void ServletDispatch::addGlobServlet(const std::string& uri,FunctionServlet::callback cb)   //添加模糊匹配servlet(回调函数)
        {//cb回调函数
            return addGlobServlet(uri, std::make_shared<FunctionServlet>(cb));  //make_shared在动态内存中分配一个对象并初始化它，返回指向此对象的shared_ptr
        }

        void ServletDispatch::delServlet(const std::string& uri)    //删除精准匹配servlet
        {
            RWMutexType::WriteLock lock(m_mutex);
            m_datas.erase(uri);
        }

        void ServletDispatch::delGlobServlet(const std::string& uri)    //删除模糊匹配servlet
        {
            RWMutexType::WriteLock lock(m_mutex);
            for(auto it = m_globs.begin();it != m_globs.end(); ++it)
            {
                if(it->first == uri)
                {
                    m_globs.erase(it);
                    break;
                }
            }
        }

        Servlet::ptr ServletDispatch::getServlet(const std::string& uri)    //通过uri获取精准匹配servlet
        {
            RWMutexType::ReadLock lock(m_mutex);
            auto it = m_datas.find(uri);
            return it == m_datas.end() ? nullptr : it->second->get();
        }

        Servlet::ptr ServletDispatch::getGlobServlet(const std::string& uri)    //通过uri获取模糊匹配servlet
        {
            RWMutexType::ReadLock lock(m_mutex);
            for(auto it = m_globs.begin();it != m_globs.end(); ++it)
            {
                if(it->first == uri)
                {
                    return it->second->get();
                }
            }
            return nullptr;
        }

        Servlet::ptr ServletDispatch::getMatchedServlet(const std::string& uri) //通过uri获取servlet(优先精准匹配,其次模糊匹配,最后返回默认)
        {
            RWMutexType::ReadLock lock(m_mutex);
            auto mit = m_datas.find(uri);   
            if(mit != m_datas.end())    //优先精准匹配
            {
                return mit->second->get();
            }
            for(auto it = m_globs.begin();it != m_globs.end(); ++it)    //其次模糊匹配
            {
                if(!fnmatch(it->first.c_str(), uri.c_str(), 0))
                {
                    return it->second->get();
                }
            }
            return m_default;   //最后返回默认
        }

        void ServletDispatch::listAllServletCreator(std::map<std::string, IServletCreator::ptr>& infos) //复制精准匹配servlet MAP
        {
            RWMutexType::ReadLock lock(m_mutex);
            for(auto& i : m_datas)
            {
                infos[i.first] = i.second;
            }
        }

        void ServletDispatch::listAllGlobServletCreator(std::map<std::string, IServletCreator::ptr>& infos) //复制模糊匹配servlet 数组
        {
            RWMutexType::ReadLock lock(m_mutex);
            for(auto& i : m_globs)
            {
                infos[i.first] = i.second;
            }
        }

        //NotFoundServlet
        NotFoundServlet::NotFoundServlet(const std::string& name)   //初始化
            :Servlet("NotFoundServlet")
            ,m_name(name)
        {
            m_content = "<html><head><title>404 Not Found"
                "</title></head><body><center><h1>404 Not Found</h1></center>"
                "<hr><center>" + name + "</center></body></html>";

        }

        int32_t NotFoundServlet::handle(sylar::http::HttpRequest::ptr request   //处理(访问错误)
                        , sylar::http::HttpResponse::ptr response
                        , sylar::http::HttpSession::ptr session)
        {
            response->setStatus(sylar::http::HttpStatus::NOT_FOUND);
            response->setHeader("Server", "sylar/1.0.0");
            response->setHeader("Content-Type", "text/html");
            response->setBody(m_content);
            return 0;
        }


    }
}
