#include"../sylar_server/config.h"
#include"../sylar_server/log.h"
#include"sylar_server/env.h"
#include <yaml-cpp/yaml.h>
#include <iostream>
    //Node 是 yaml-cpp 中的核心概念，是最重要的数据结构
    /*Null 空节点
    Type是yaml格式的类型
    Sequence 序列，类似于一个Vector,对应YAML格式中的数组
    Map 类似标准库中的Map，对应YAML格式中的对象
    Scalar 标量，对应YAML格式中的常量*/

    //YAML：：Node是静态全局变量

//各种STL类型
//创建int类型配置对象
sylar::ConfigVar<int>::ptr g_int_value_config=
    sylar::Config::Lookup("system.port",(int)8080,"system port");   

//创建float类型配置对象
sylar::ConfigVar<float>::ptr g_int_valuex_config=
    sylar::Config::Lookup("system.port",(float)8080,"system port"); 

//创建float类型配置对象
sylar::ConfigVar<float>::ptr g_float_value_config=
    sylar::Config::Lookup("system.value",(float)10.2f,"system value");

//创建vector<int>类型配置对象
sylar::ConfigVar<std::vector<int>>::ptr g_int_vec_value_config=
    sylar::Config::Lookup("system.int_vec",std::vector<int>{1,2},"system int vec");

//创建list<int>类型配置对象
sylar::ConfigVar<std::list<int>>::ptr g_int_list_value_config=
    sylar::Config::Lookup("system.int_list",std::list<int>{1,2},"system int list");

//创建set<int>类型配置对象
sylar::ConfigVar<std::set<int>>::ptr g_int_set_value_config=
    sylar::Config::Lookup("system.int_set",std::set<int>{1,2},"system int set");

//创建unordered_set<int>类型配置对象
sylar::ConfigVar<std::unordered_set<int>>::ptr g_int_uset_value_config=
    sylar::Config::Lookup("system.int_uset",std::unordered_set<int>{1,2},"system int uset");

//创建map<std::string,int>类型配置对象
sylar::ConfigVar<std::map<std::string,int>>::ptr g_str_int_map_value_config=
    sylar::Config::Lookup("system.str_int_map",std::map<std::string,int>{{"k",2}},"system str int map");

//创建unordered_map<std::string,int>类型配置对象
sylar::ConfigVar<std::unordered_map<std::string,int>>::ptr g_str_int_umap_value_config=
    sylar::Config::Lookup("system.str_int_umap",std::unordered_map<std::string,int>{{"k",2}},"system str int umap");

void print_yaml(const YAML::Node &node,int level)   //解析yaml文件
{
    if(node.IsScalar()) //yaml常量存在
    {
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT())<<std::string(level*4,' ')
            <<node.Scalar()<<"-"<<node.Type()<<"-"<<level;
    }
    else if(node.IsNull()) //yaml存在空节点
    {
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT())<<std::string(level*4,' ')
            <<"NULL -"<<node.Type()<<"-"<<level;
    }
    else if(node.IsMap()) //yaml二叉树存在数据
    {
        for(auto it=node.begin();it!=node.end();++it)
        {
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT())<<std::string(level*4,' ')
                <<it->first<<"-"<<it->second.Type()<<"-"<<level;
            print_yaml(it->second,level+1);
        }
    }
    else if(node.IsSequence()) //yaml数组存在数据
    {
        for(size_t i=0;i<node.size();++i)
        {
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT())<<std::string(level*4,' ')
                <<i<<"-"<<node[i].Type()<<"-"<<level;
            print_yaml(node[i],level+1);
        }
    }
}

void test_yaml()    //要加载的yaml的路径   /home/yangxin/sylar-master/bin/conf/log.yml
{
    YAML::Node root=YAML::LoadFile("/home/yangxin/sylar-master/bin/conf/log.yml");

    print_yaml(root,0);

    //SYLAR_LOG_INFO(SYLAR_LOG_ROOT())<<root;
}

//各种STL类型
void test_config()  //config配置管理函数
{
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT())<<"before:"<<g_int_value_config->getValue();
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT())<<"before:"<<g_float_value_config->toString();

//其他类型
#define XX(g_var,name,prefix) \
    { \
        auto& v=g_var->getValue(); \
        for(auto& i : v) \
        { \
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT())<<#prefix" "#name": "<<i; \
        } \
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT())<<#prefix" "#name"yaml: "<<g_var->toString(); \
    }

//map<std::string,int>类型————unordered_map<std::string,int>类型
#define XX_M(g_var,name,prefix) \ 
    { \
        auto& v=g_var->getValue(); \
        for(auto& i : v) \
        { \
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT())<<#prefix" "#name": {" \
                    <<i.first<<"-"<<i.second<<"}"; \
        } \
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT())<<#prefix" "#name"yaml: "<<g_var->toString(); \
    }

    XX(g_int_vec_value_config,int_vec,before);
    XX(g_int_list_value_config,int_list,before);
    XX(g_int_set_value_config,int_set,before);
    XX(g_int_uset_value_config,int_uset,before);
    XX_M(g_str_int_map_value_config,str_int_map,before);
    XX_M(g_str_int_umap_value_config,str_int_umap,before);

    YAML::Node root=YAML::LoadFile("/home/yangxin/sylar-master/bin/conf/log.yml");  //LoadFile（）将yaml文件生成Node形式
    sylar::Config::LoadFromYaml(root);

    SYLAR_LOG_INFO(SYLAR_LOG_ROOT())<<"after:"<<g_int_value_config->getValue();
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT())<<"after:"<<g_float_value_config->toString();

    XX(g_int_vec_value_config,int_vec,after);
    XX(g_int_list_value_config,int_list,after);
    XX(g_int_set_value_config,int_set,after);
    XX(g_int_uset_value_config,int_uset,after);
    XX_M(g_str_int_map_value_config,str_int_map,after);
    XX_M(g_str_int_umap_value_config,str_int_umap,after);
}


//自定义类型
class Person
{
public:
    Person() {};
    std::string m_name;
    int m_age=0;
    bool m_sex=0;

    std::string toString() const
    {
        std::stringstream ss;
        ss<<"[Person name="<<m_name
          <<" age="<<m_age
          <<" sex="<<m_sex
          <<"]";
        return ss.str();
    }

    //判断是否相等
    bool operator==(const Person &oth) const
    {
        return m_name==oth.m_name
            && m_age==oth.m_age
            && m_sex==oth.m_sex;
    }

};

namespace sylar
{
    template<>
    class LexicalCast<std::string,Person>   //类型转换模板类片特化(std::string 转换成 自定义类型Person)
    {
    public:
        Person operator()(const std::string& v)
        {
            YAML::Node node=YAML::Load(v);  //将string类型转化成yaml字典
            Person p;
            p.m_name=node["name"].as<std::string>();
            p.m_age=node["age"].as<int>();
            p.m_sex=node["sex"].as<bool>();
            return p;
        }
    };

    template<>
    class LexicalCast<Person,std::string>   //类型转换模板类片特化(自定义类型Person 转换成 std::string)
    {
    public:
        std::string operator()(const Person& p)
        {
            YAML::Node node;  
            node["name"]=p.m_name;
            node["age"]=p.m_age;
            node["sex"]=p.m_sex;
            std::stringstream ss;
            ss<<node;
            return ss.str();
        }
    };
}

//创建person类配置对象
sylar::ConfigVar<Person>::ptr g_person=
    sylar::Config::Lookup("class.person",Person(),"system person");  

//创建std::map<std::string,Person>类配置对象
sylar::ConfigVar<std::map<std::string,Person>>::ptr g_person_map=
    sylar::Config::Lookup("class.map",std::map<std::string,Person>(),"system person");  

//创建std::map<std::string,Person>类配置对象
sylar::ConfigVar<std::map<std::string,std::vector<Person>>>::ptr g_person_vec_map=
    sylar::Config::Lookup("class.vec_map",std::map<std::string,std::vector<Person>>(),"system person");  

void test_class()   //自定义类型处理
{
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT())<<"before: "<<g_person->getValue().toString()<<"-"<<g_person->toString();
    
#define XX_PM(g_var,prefix) \
    { \
        auto m=g_person_map->getValue(); \
        for(auto& i : m) \
        { \
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT())<<prefix<<": "<<i.first<<"-"<<i.second.toString(); \
        } \
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT())<<prefix<<": size="<<m.size(); \
    }

    g_person->addListener([](const Person& old_value,const Person& new_value){   //回调函数测试
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT())<<"old_value="<<old_value.toString()
            <<" new_value="<<new_value.toString();
    });

    XX_PM(g_person_map,"class.map before");
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT())<<"begin: "<<g_person_vec_map->toString();

    YAML::Node root=YAML::LoadFile("/home/yangxin/sylar-master/bin/conf/test.yml");  //LoadFile（）将yaml文件生成Node形式
    sylar::Config::LoadFromYaml(root);

    SYLAR_LOG_INFO(SYLAR_LOG_ROOT())<<"after: "<<g_person->getValue().toString()<<"-"<<g_person->toString();
    XX_PM(g_person_map,"class.map after");
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT())<<"after: "<<g_person_vec_map->toString();
}

void test_log()
{
    static sylar::Logger::ptr system_log=SYLAR_LOG_NAME("system");  //找到system日志器
    SYLAR_LOG_INFO(system_log)<<"hello system"<<std::endl;  //存进system日志文件
    std::cout<<sylar::LoggerMgr::GetInstance()->toYamlString()<<std::endl;  //打印默认值配置参数

    YAML::Node root=YAML::LoadFile("/home/yangxin/sylar-master/bin/conf/log.yml");
    sylar::Config::LoadFromYaml(root);  //使用YAML::Node初始化配置模块————配合yaml文件使用******事件变更

    std::cout<<"======================"<<std::endl;
    std::cout<<sylar::LoggerMgr::GetInstance()->toYamlString()<<std::endl;  //事件变更之后******用日志器里的yaml::node解析log.txt并打印————YAML：：Node是静态全局变量
    std::cout<<"======================"<<std::endl;
    std::cout<<root<<std::endl; //打印log.yml所有

    SYLAR_LOG_INFO(system_log)<<"hello system"<<std::endl;

    //_____________________________
    system_log->setFormatter("%d - %m%n");  //更改日志，模板
    SYLAR_LOG_INFO(system_log)<<"hello system"<<std::endl;
}

void test_loadconf()
{
    sylar::Config::LoadFromConfDir("conf");
}

int main(int argc,char ** argv)
{
    //test_yaml();  //可以运行    全部类型都可以
    //test_config(); //不可以运行     各种STL类型都行  （如果是输出不出的话，是要没有进程在运行（重开）————猜测）test_config和test_class二选一
    //test_class();  //不可以运行     自定义类型  （如果是输出不出的话，是要没有进程在运行（重开）————猜测）
    test_log();
    // sylar::EnvMgr::GetInstance()->init(argc,argv);
    // test_loadconf();
    // std::cout<<"============"<<std::endl;
    // test_loadconf();
    // return 0;

    // sylar::Config::Visit([](sylar::ConfigVarBase::ptr var){
    //     SYLAR_LOG_INFO(SYLAR_LOG_ROOT())<<"name="<<var->getName()
    //                             <<" description="<<var->getDescription()
    //                             <<" typename="<<var->getTypeName()
    //                             <<" value="<<var->toString();
    // });
    return 0;
}