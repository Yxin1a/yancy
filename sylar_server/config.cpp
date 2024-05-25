#include"../sylar_server/config.h"
#include"../sylar_server/log.h"
#include <yaml-cpp/yaml.h>
#include"sylar_server/env.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

    //Node 是 yaml-cpp 中的核心概念，是最重要的数据结构
    /*Null 空节点
    Sequence 序列，类似于一个Vector,对应YAML格式中的数组
    Map 类似标准库中的Map，对应YAML格式中的对象(使用begin和end迭代器)
    Scalar 标量，对应YAML格式中的常量*/

    //YAML：：Node是静态全局变量

namespace sylar
{
    static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

    //Config::ConfigVarMap Config::s_datas;   //所有的配置项(是ConfigVarMap类型————map字典类型)

    ConfigVarBase::ptr Config::LookupBase(const std::string& name)  //查找配置参数,返回配置参数的基类
    {
        RWMutexType::ReadLock lock(GetMutex());     //互斥量
        auto it=GetDatas().find(name);
        return it==GetDatas().end() ? nullptr : it->second;
    }

    // "A.B", 10
    // A:
    //    B: 10
    //    C: str

    static void ListAllMember(const std::string& prefix,
                              const YAML::Node& node,
                              std::list<std::pair<std::string, const YAML::Node> >& output) //将合理的数据插入list容器（模板）
    {
        if(prefix.find_first_not_of("abcdefghikjlmnopqrstuvwxyz._012345678")!= std::string::npos)   //find_first_not_of()搜索与其参数中指定的任何字符不匹配的第一个字符
        {
            SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "Config invalid name: " << prefix << " : " << node;
            return;
        }
        output.push_back(std::make_pair(prefix, node));
        if(node.IsMap())    //IsMap类似标准库中的Map（二叉树），对应YAML格式中的对象 
        {
            for(auto it = node.begin();it != node.end(); ++it) 
            {
                ListAllMember(prefix.empty() ? it->first.Scalar(): prefix + "." + it->first.Scalar(), it->second, output);
            }
        }
    }

    void Config::LoadFromYaml(const YAML::Node &root)   //使用YAML::Node初始化配置模块
    {
        std::list<std::pair<std::string, const YAML::Node> > all_nodes;     //pair容器数据成对存在
        ListAllMember("", root, all_nodes);//将合理的数据插入list容器（all_nodes模板）

        for(auto& i : all_nodes) 
        {
            std::string key = i.first;
            if(key.empty()) 
            {
                continue;
            }

            std::transform(key.begin(), key.end(), key.begin(), ::tolower); //std::transform被用于将字符串m_name中的字母字符转换为小写形式并覆盖原来的字符串。所以不区分大小写
            ConfigVarBase::ptr var=LookupBase(key);

            if(var) //如果找到了
            {
                if(i.second.IsScalar())     //IsScalar是否为标量
                {
                    var->fromString(i.second.Scalar()); //从YAML String 转成参数值
                }
                else    //  转到string里面去
                {
                    std::stringstream ss;   //输出流
                    ss<<i.second;
                    var->fromString(ss.str());  //从YAML String 转成参数值
                }
            }
        }
    }

    static std::map<std::string, uint64_t> s_file2modifytime;   //上一个文件修改时间
    static sylar::Mutex s_mutex;    //互斥量

    void Config::LoadFromConfDir(const std::string& path, bool force)   //加载path文件夹里面的配置文件.yml————将所有yml文件转换成对应的类型(如果配置文件没有变化，则不用加载该文件)
    {
        std::string absoulte_path = sylar::EnvMgr::GetInstance()->getAbsolutePath(path);    //获取path绝对路径
        std::vector<std::string> files;
        FSUtil::ListAllFile(files, absoulte_path, ".yml");

        for(auto& i : files)
        {
            {   //如果配置没有变化，则不用多次path文件夹里面的配置文件.yml
                struct stat st;
                lstat(i.c_str(), &st);   //获取文件修改时间
                sylar::Mutex::Lock lock(s_mutex);
                if(!force && s_file2modifytime[i] == (uint64_t)st.st_mtime) //文件没有修改，配置没有变化
                {
                    continue;
                }
                s_file2modifytime[i] = st.st_mtime;
            }
            try
            {
                YAML::Node root = YAML::LoadFile(i);    //加载i的(yml)文件
                LoadFromYaml(root);
                SYLAR_LOG_INFO(g_logger) << "LoadConfFile file="
                    << i << " ok";
            }
            catch (...) //有问题
            {
                SYLAR_LOG_ERROR(g_logger) << "LoadConfFile file="
                    << i << " failed";
            }
        }
    }

    void Config::Visit(std::function<void(ConfigVarBase::ptr)> cb)   //遍历配置模块里面所有配置项————ConfigVarBase::ptr是被调用函数的形参类型
    {
        RWMutexType::ReadLock lock(GetMutex());     //互斥量
        ConfigVarMap& m=GetDatas();     //获取所有的配置项
        for(auto it=m.begin();it!=m.end();++it)
        {
            cb(it->second);
        }
    }

}