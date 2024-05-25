#include"servlet_path.h"
#include"../log.h"

namespace sylar
{
    namespace http
    {
        
        static sylar::Logger::ptr g_logger=SYLAR_LOG_NAME("system");

        //ServletPath
        ServletPath::ServletPath(ServletPathConf conf) //初始化
            :m_conf(conf)
        {}

        //ServletPathHtml
        ServletPathHtml::ServletPathHtml(ServletPathConf conf) //初始化
            :ServletPath(conf)
        {}

        static bool FileIfream(std::stringstream &ss,std::string path)  //读取path路径文件的数据到ss中··
        {
            std::ifstream ifs;
            ifs.open(path,std::ios::in);
            if(!ifs)
            {
                SYLAR_LOG_INFO(g_logger)<<path<<" html file is exist";
                return false;
            }
            std::string buf;
            while(getline(ifs,buf))
            {
                ss<<buf;
            }
            return true;
        }

        bool ServletPathHtml::fun(sylar::http::ServletDispatch::ptr& dispatch)  //定义servlet匹配的回调函数
        {
            if(m_conf.type!="html")
            {
                SYLAR_LOG_ERROR(g_logger)<<"servlet type error";
                return false;
            }
            dispatch->addServlet(m_conf.uri,[](sylar::http::HttpRequest::ptr req    //添加精准匹配servlet(回调函数)
                    ,sylar::http::HttpResponse::ptr rsp
                    ,sylar::http::HttpSession::ptr session)
            {
                std::string path="/home/yangxin/sylar-master"+req->getPath()+".html";
                std::stringstream ss;
                if(!FileIfream(ss,path))
                {
                    return false;
                }
                rsp->setBody(ss.str());
            });
            return true;
        }

    }
}