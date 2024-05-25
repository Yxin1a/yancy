//配置模块————约定优于配置
/*显示服务器各种系统的配置，比如日志系统*/
//对应yaml文件

//yaml文件 ——解析——> yaml::node格式 ——解析——>程序各种类型的格式（string）
//YAML：：Node是静态全局变量

#ifndef __SYLAR_CONFIG_H__
#define __SYLAR_CONFIG_H__

#include<memory>  //智能指针
#include<sstream> //流
#include<boost/lexical_cast.hpp>//类型转换
#include<exception>
#include<string>
#include<yaml-cpp/yaml.h>
#include<functional>

#include<vector>
#include<list>
#include<map>
#include<set>
#include<unordered_map>
#include<unordered_set>

#include"log.h"
#include"thread.h"

namespace sylar
{

    /**
     *  配置变量的基类
     */
    class ConfigVarBase
    {
    public:
        typedef std::shared_ptr<ConfigVarBase> ptr;

        /**
         *  构造函数
         *  name 配置参数名称[0-9a-z_.]
         *  description 配置参数描述
         */
        ConfigVarBase(const std::string& name, const std::string& description = "")
            :m_name(name)
            , m_description(description) 
            {
                //std::transform被用于将字符串m_name中的字母字符转换为小写形式并覆盖原来的字符串。所以不区分大小写
                std::transform(m_name.begin(), m_name.end(), m_name.begin(), ::tolower);    
                //将某操作(tolower)应用于指定范围的每个元素
                //tolower   若参数  为大写字母则将该对应的小写字母返回。
            }

        /**
         *  析构函数(方便释放内存)
         */
        virtual ~ConfigVarBase() {}

        const std::string& getName() const { return m_name; }    //返回配置参数名称
        const std::string& getDescription() const { return m_description; }  //返回配置参数的描述

        virtual std::string toString() = 0;   //转成字符串（string）
        virtual bool fromString(const std::string& val) = 0;    //从string转化为相应类型

        virtual std::string getTypeName() const=0;  //返回配置参数值的类型名称
    private:
        std::string m_name; // 配置参数的名称
        std::string m_description;// 配置参数的描述
    };

    /**
     *  简单类型转换模板类(F 源类型, T 目标类型)
     */
    //F from_type ——> T to_Type
    template<class F,class T>
    class LexicalCast
    {
    public:
        T operator()(const F &v)
        {
            return boost::lexical_cast<T>(v);
        }
    };

    /**
     *  类型转换模板类片特化(YAML String 转换成 std::vector<T>)
     */
    template<class T>
    class LexicalCast<std::string,std::vector<T>>   //模块重载
    {
    public:
        std::vector<T> operator()(const std::string& v)
        {
            YAML::Node node=YAML::Load(v);  //将string类型转化成yaml数组
            typename std::vector<T> vec;    //typename用于指出模板声明（或定义）中的非独立名称是类型名
            std::stringstream ss;
            for(size_t i=0;i<node.size();++i)   //将yaml数组转换成vector类型
            {
                ss.str("");
                ss<<node[i];
                vec.push_back(LexicalCast<std::string,T>()(ss.str()));
            }
            return vec;
        }
    };

    /**
     *  类型转换模板类片特化(std::vector<T> 转换成 YAML String)
     */
    template<class T>
    class LexicalCast<std::vector<T>,std::string>   //模块重载
    {
    public:
        std::string operator()(const std::vector<T>& v)
        {
            YAML::Node node;  
            for(auto &i : v)    //将vector类型转化成yaml数组
            {
                node.push_back(YAML::Load(LexicalCast<T,std::string>()(i)));
            }
            std::stringstream ss;
            ss<<node;
            return ss.str();
        }
    };

    /**
     *  类型转换模板类片特化(YAML String 转换成 std::list<T>)
     */
    template<class T>
    class LexicalCast<std::string,std::list<T>>   //模块重载
    {
    public:
        std::list<T> operator()(const std::string& v)
        {
            YAML::Node node=YAML::Load(v);  //将string类型转化成yaml数组
            typename std::list<T> vec;    //typename用于指出模板声明（或定义）中的非独立名称（dependent names）是类型名
            std::stringstream ss;
            for(size_t i=0;i<node.size();++i)   //将yaml数组转换成list类型
            {
                ss.str("");
                ss<<node[i];
                vec.push_back(LexicalCast<std::string,T>()(ss.str()));
            }
            return vec;
        }
    };

    /**
     *  类型转换模板类片特化(std::list<T> 转换成 YAML String)
     */
    template<class T>
    class LexicalCast<std::list<T>,std::string>   //模块重载
    {
    public:
        std::string operator()(const std::list<T>& v)
        {
            YAML::Node node;  
            for(auto &i : v)    //将list类型转化成yaml数组
            {
                node.push_back(YAML::Load(LexicalCast<T,std::string>()(i)));
            }
            std::stringstream ss;
            ss<<node;
            return ss.str();
        }
    };

    /**
     *  类型转换模板类片特化(YAML String 转换成 std::set<T>)
     */
    template<class T>
    class LexicalCast<std::string,std::set<T>>   //模块重载
    {
    public:
        std::set<T> operator()(const std::string& v)
        {
            YAML::Node node=YAML::Load(v);  //将string类型转化成yaml数组
            typename std::set<T> vec;    //typename用于指出模板声明（或定义）中的非独立名称（dependent names）是类型名
            std::stringstream ss;
            for(size_t i=0;i<node.size();++i)   //将yaml数组转换成set类型
            {
                ss.str("");
                ss<<node[i];
                vec.insert(LexicalCast<std::string,T>()(ss.str()));
            }
            return vec;
        }
    };

    /**
     *  类型转换模板类片特化(std::set<T> 转换成 YAML String)
     */
    template<class T>
    class LexicalCast<std::set<T>,std::string>   //模块重载
    {
    public:
        std::string operator()(const std::set<T>& v)
        {
            YAML::Node node;  
            for(auto &i : v)    //将set类型转化成yaml数组
            {
                node.push_back(YAML::Load(LexicalCast<T,std::string>()(i)));
            }
            std::stringstream ss;
            ss<<node;
            return ss.str();
        }
    };

    /**
     *  类型转换模板类片特化(YAML String 转换成 std::unordered_set<T>)
     */
    template<class T>
    class LexicalCast<std::string,std::unordered_set<T>>   //模块重载
    {
    public:
        std::unordered_set<T> operator()(const std::string& v)
        {
            YAML::Node node=YAML::Load(v);  //将string类型转化成yaml数组
            typename std::unordered_set<T> vec;    //typename用于指出模板声明（或定义）中的非独立名称（dependent names）是类型名
            std::stringstream ss;
            for(size_t i=0;i<node.size();++i)   //将yaml数组转换成unordered_set类型
            {
                ss.str("");
                ss<<node[i];
                vec.insert(LexicalCast<std::string,T>()(ss.str()));
            }
            return vec;
        }
    };

    /**
     *  类型转换模板类片特化(std::unordered_set<T> 转换成 YAML String)
     */
    template<class T>
    class LexicalCast<std::unordered_set<T>,std::string>   //模块重载
    {
    public:
        std::string operator()(const std::unordered_set<T>& v)
        {
            YAML::Node node;  
            for(auto &i : v)    //将unordered_set类型转化成yaml数组
            {
                node.push_back(YAML::Load(LexicalCast<T,std::string>()(i)));
            }
            std::stringstream ss;
            ss<<node;
            return ss.str();
        }
    };

    /**
     *  类型转换模板类片特化(YAML String 转换成 std::map<std::string,T>)
     */
    template<class T>
    class LexicalCast<std::string,std::map<std::string,T>>   //模块重载
    {
    public:
        std::map<std::string,T> operator()(const std::string& v)
        {
            YAML::Node node=YAML::Load(v);  //将string类型转化成yaml字典
            typename std::map<std::string,T> vec;    //typename用于指出模板声明（或定义）中的非独立名称（dependent names）是类型名
            std::stringstream ss;
            for(auto it =node.begin();it!=node.end();++it)   //将yaml数组转换成map类型
            {
                ss.str("");
                ss<<it->second;
                vec.insert(std::make_pair(it->first.Scalar(),LexicalCast<std::string,T>()(ss.str())));  //make_pair（）用于构造具有两个元素的对象，这里构造了字典
            }
            return vec;
        }
    };

    /**
     *  类型转换模板类片特化(std::map<std::string,T> 转换成 YAML String)
     */
    template<class T>
    class LexicalCast<std::map<std::string,T>,std::string>   //模块重载
    {
    public:
        std::string operator()(const std::map<std::string,T>& v)
        {
            YAML::Node node;  
            for(auto &i : v)    //将map类型转化成yaml字典
            {
                node[i.first]=YAML::Load(LexicalCast<T,std::string>()(i.second));
            }
            std::stringstream ss;
            ss<<node;
            return ss.str();
        }
    };

    /**
     *  类型转换模板类片特化(YAML String 转换成 std::unordered_map<std::string,T>)
     */
    template<class T>
    class LexicalCast<std::string,std::unordered_map<std::string,T>>   //模块重载
    {
    public:
        std::unordered_map<std::string,T> operator()(const std::string& v)
        {
            YAML::Node node=YAML::Load(v);  //将string类型转化成yaml字典
            typename std::unordered_map<std::string,T> vec;    //typename用于指出模板声明（或定义）中的非独立名称（dependent names）是类型名
            std::stringstream ss;
            for(auto it =node.begin();it!=node.end();++it)   //将yaml数组转换成unordered_map类型
            {
                ss.str("");
                ss<<it->second;
                vec.insert(std::make_pair(it->first.Scalar(),LexicalCast<std::string,T>()(ss.str())));  //make_pair（）用于构造具有两个元素的对象，这里构造了字典
            }
            return vec;
        }
    };

    /**
     *  类型转换模板类片特化(std::unordered_map<std::string,T> 转换成 YAML String)
     */
    template<class T>
    class LexicalCast<std::unordered_map<std::string,T>,std::string>   //模块重载
    {
    public:
        std::string operator()(const std::unordered_map<std::string,T>& v)
        {
            YAML::Node node;  
            for(auto &i : v)    //将unordered_map类型转化成yaml字典
            {
                node[i.first]=YAML::Load(LexicalCast<T,std::string>()(i.second));
            }
            std::stringstream ss;
            ss<<node;
            return ss.str();
        }
    };

    /**
     *  配置参数模板子类,保存对应类型的参数值
     *          T 参数的具体类型
     *          FromStr 从std::string转换成T类型的仿函数
     *          ToStr 从T转换成std::string的仿函数
     *          std::string 为YAML格式的字符串(yaml-cpp)
     */
    template<class T,class FromStr=LexicalCast<std::string,T>,class ToStr=LexicalCast<T,std::string>>
    class ConfigVar :public ConfigVarBase
    {
    public:
        typedef RWMutex RWMutexType;
        typedef std::shared_ptr<ConfigVar> ptr;
        typedef std::function<void (const T& old_value,const T& new_value)> on_change_cb;   //function把函数签名包装成满足结构的接口(回调函数)
        /*回调函数就是一个通过函数指针调用的函数。
        如果你把函数的指针（地址）作为参数传递给另一个函数，当这个指针被用来调用其所指向的函数时，我们就说这是回调函数。*/

        /**
         *  通过参数名,参数值,描述构造ConfigVar
         *      name 参数名称有效字符为[0-9a-z_.]   （父类）
         *      default_value 参数的默认值          （子类）
         *      description 参数的描述              （父类）
         */
        ConfigVar(const std::string& name   //参数名称有效字符为[0-9a-z_.]
            , const T& default_value        //参数的默认值
            , const std::string& description = "")  //参数的描述
            :ConfigVarBase(name, description)   //调用配置变量基类的构造函数
            , m_val(default_value) 
            {}

        /**
         *  将参数值转换成YAML String
         *  当转换失败抛出异常
         */
        std::string toString() override
        {
            try //异常处理  try里放可能引发异常的代码块
            {
                RWMutexType::ReadLock lock(m_mutex);    //互斥量
                //return boost::lexical_cast<std::string>(m_val);     //lexical_cast<>函数将 参数 转化成 对应的类型
                return ToStr()(m_val);  //将参数值转换成YAML String
            }
            catch (std::exception& e)   //catch引发特定异常时执行的代码块   exception抓异常信息
            {
                SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "ConfigVar::toString exception"
                    << e.what() << "convert:" << typeid(m_val).name()<< "to string";
                    //<<"name="<<m_name;
                    //what()用于获取字符串标识异常。此函数返回一个空终止的字符序列，该序列可用于标识异常
                    //typeid()用来获取一个表达式的类型信息。(typeid(m_val)是ConfigVarBase类型)
            }
            return "";
        }

        /**
         *  从YAML String 转成参数值
         *  当转换失败抛出异常
         */
        bool fromString(const std::string& val) override
        {
            try//异常处理  try里放可能引发异常的代码块
            {
                //m_val = boost::lexical_cast<T>(val);    //lexical_cast<>函数将string转化成参数值
                setValue(FromStr()(val));     //从YAML String 转成参数值
            }
            catch (std::exception& e)   //catch引发特定异常时执行的代码块   exception抓异常信息
            {
                SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "ConfigVar::toString exception"
                    << e.what() << "convert:string to" << typeid(m_val).name()<<"-"<<val;
                    //what()用于获取字符串标识异常。此函数返回一个空终止的字符序列，该序列可用于标识异常
                    //typeid()用来获取一个表达式的类型信息。(typeid(m_val)是ConfigVarBase类型)
            }
            return false;
        }

        /**
         * 获取当前参数值
         */
        const T getValue()
        { 
            RWMutexType::ReadLock lock(m_mutex);    //互斥量
            return m_val;
        }

        /**
         * 设置当前参数值
         * 如果参数的值有发生变化,则通知对应的注册回调函数
         */
        void setValue(const T& v) 
        {
            {
                RWMutexType::ReadLock lock(m_mutex);    //互斥量
                if(v==m_val)    //没有发生变化
                {
                    return;
                }
                for(auto & i:m_cbs) //通知（赋值）on_change_cb
                {
                    i.second(m_val,v); 
                }
            }
            RWMutexType::WriteLock lock(m_mutex);    //互斥量
            m_val=v;
        }

        /**
         *  返回配置参数值的类型名称
         */
        std::string getTypeName() const override {return typeid(T).name();}

        /**
         *  添加变化回调函数
         *  返回该回调函数对应的唯一id,用于删除回调
         */
        uint64_t addListener(on_change_cb cb)
        {
            static uint64_t s_fun_id=0;
            RWMutexType::WriteLock lock(m_mutex);    //互斥量——静态全局变量
            ++s_fun_id;
            m_cbs[s_fun_id]=cb;
            return s_fun_id;
            
        }
        
        /**
         *  删除回调函数
         *      key 回调函数的唯一id
         */
        void delListener(uint64_t key)
        {
            RWMutexType::WriteLock lock(m_mutex);   //互斥量
            m_cbs.erase(key);
        }
        
        /**
         *  获取回调函数
         *      key 回调函数的唯一id
         *      如果存在返回对应的回调函数,否则返回nullptr
         */
        on_change_cb getListener(uint64_t key)
        {
            RWMutexType::ReadLock lock(m_mutex);    //互斥量
            auto it=m_cbs.find(key);
            return it==m_cbs.end()?nullptr:it->second;
        }

        /**
         *  清理所有的回调函数
         */
        void clearListener()
        {
            RWMutexType::ReadLock lock(m_mutex);    //互斥量
            m_cbs.clear();
        }
    
    private:
    
        RWMutexType m_mutex;    //互斥量（读写共享数据才要加锁————map<uint64_t,on_change_cb>回调函数——T模板类）
        T m_val;    //参数值

        //变更回调函数组 uint64_t key要求唯一，一般可以用hash值
        std::map<uint64_t,on_change_cb> m_cbs; 
    };

    /**
     *  ConfigVar的管理类
     *  提供便捷的方法创建/访问ConfigVar
     */
    class Config
    {
    public:
        typedef std::unordered_map<std::string, ConfigVarBase::ptr> ConfigVarMap;   //用于存储由键值和映射值的组合形成的元素。
        typedef RWMutex RWMutexType;

        /**
         *  获取/创建对应参数名的配置参数
         *  name 配置参数名称
         *  default_value 参数默认值
         *  description 参数描述
         *           获取参数名为name的配置参数,如果存在直接返回
         *          如果不存在,创建参数配置并用default_value赋值
         *  返回对应的配置参数,如果参数名存在但是类型不匹配则返回nullptr
         * 如果参数名包含非法字符[^0-9a-z_.] 抛出异常 std::invalid_argument
         */
        template<class T>  
        static typename ConfigVar<T>::ptr Lookup(const std::string& name,//typename是告诉编译器这是类型，因为有一些情况：：后面是变量名
            const T& default_value, const std::string& description = "")
            {
                RWMutexType::WriteLock lock(GetMutex());     //互斥量
                auto it =GetDatas().find(name);    //用于在无序列图中搜索特定键。--如果给定的键存在于unordered_map则返回该元素的迭代器，否则返回映射迭代器的末尾。
                if(it!=GetDatas().end())
                {
                    auto tmp=std::dynamic_pointer_cast<ConfigVar<T> >(it->second);
                    if (tmp)    //该配置参数名称已存在
                    {
                        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "Lookup name=" << name << "exists";
                        return tmp;
                    }
                    else
                    {
                        SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "Lookup name=" << name << "exists but type not"
                                <<typeid(T).name()<<"real_type="<<it->second->getTypeName() ////typeid()用来获取一个表达式的类型信息。(typeid(m_val)是ConfigVarBase类型)
                                <<" "<<it->second->toString();
                        return nullptr;
                    }
                }
                if (name.find_first_not_of("abcdefghijklmnopqrstuvwxyz._012345678") != std::string::npos)   //find_first_not_of()搜索与其参数中指定的任何字符不匹配的第一个字符
                { //该配置参数名称无效                                                                       //成功时第一个不匹配字符的索引，如果未找到此类字符，则返回字符串：：npos
                    SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "Lookup name invalid" << name;
                    throw std::invalid_argument(name);
                }
                typename ConfigVar<T>::ptr v(new ConfigVar<T>(name, default_value, description));   //创建配置参数
                GetDatas()[name] = v;
                return v;
            }

        /**
         *  查找配置参数
         *      name 配置参数名称
         *  返回配置参数名为name的配置参数
         */
        template<class T>
        static typename ConfigVar<T>::ptr Lookup(const std::string& name)
        {
            RWMutexType::ReadLock lock(GetMutex());     //互斥量
            auto it = GetDatas().find(name);   //用于在无序列图中搜索特定键。--如果给定的键存在于unordered_map则返回该元素的迭代器，否则返回映射迭代器的末尾。
            if (it == GetDatas().end())
            {
                return nullptr;
            }
            return std::dynamic_pointer_cast<ConfigVar<T>>(it->second);     //dynamic_pointer_cast———将基类的指针或引用安全地转换成派生类的指针或引用，并用派生类的指针或引用调用非虚函数
        }

        /**
         *  使用YAML::Node初始化配置模块
         */
        static void LoadFromYaml(const YAML::Node &root);

        /**
         *  //加载path文件夹里面的配置文件.yml————将所有yml文件转换成对应的类型(如果配置文件没有变化，则不用加载该文件)
         */
        static void LoadFromConfDir(const std::string& path, bool force = false);

        /**
         *  查找配置参数,返回配置参数的基类
         *  name 配置参数名称
         */
        static ConfigVarBase::ptr LookupBase(const std::string& name);

        /**
         *  遍历配置模块里面所有配置项
         *      cb 配置项回调函数
         */
        static void Visit(std::function<void(ConfigVarBase::ptr)> cb);
    private:
        static ConfigVarMap& GetDatas()     //获取所有的配置项
        {
            static ConfigVarMap s_datas;    //所有的配置项(是ConfigVarMap类型————map字典类型)
            return s_datas;
        }
    
        static RWMutexType& GetMutex()      //获取互斥量————因为这个类基本都是创建全局变量 和 静态全局变量
        {//目地：(锁要比其他全局变量先创建,而静态变量优先级最先被初始化)
            static RWMutexType s_mutex;    //互斥量（读写共享数据才要加锁———static ConfigVarMap静态全局变量——全局变量）
            return s_mutex;
        }
    };


}

#endif