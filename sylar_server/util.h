//  常用的工具函数

#ifndef __SYLAR_UTIL_H__
#define __SYLAR_UTIL_H__

#include <cxxabi.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <stdint.h>
#include<string>
#include<vector>
#include<boost/lexical_cast.hpp>

namespace sylar
{
    pid_t GetThreadId();    //返回当前线程ID

    u_int32_t GetFiberId(); //返回当前协程ID

    /**
     *  获取当前的调用栈
     *      bt 保存调用栈
     *      size 最多返回层数
     *      skip 跳过栈顶的层数
     */
    void Backtrace(std::vector<std::string>& bt,int size=64,int skip=1);

    /**
     *  获取当前栈信息的字符串
     *      size 栈的最大层数
     *      skip 跳过栈顶的层数
     *      prefix 栈信息前输出的内容
     */
    std::string BacktraceToString(int size=64,int skip=2,const std::string& prefix="");

    /**
     *  获取当前时间的毫秒级
     */
    uint64_t GetCurrentMS();

    /**
     *  获取当前时间的微秒级
     */
    uint64_t GetCurrentUS();

    /**
     *  将时间戳以format格式返回字符串
    */
    std::string Time2Str(time_t ts = time(0), const std::string& format = "%Y-%m-%d %H:%M:%S");

    /**
     *  将以format格式的时间字符串返回对应时间戳
    */
    time_t Str2Time(const std::string& timeStr,const std::string& format = "%Y-%m-%d %H:%M:%S");

    /**
     * 文件相关的操作
    */
    class FSUtil
    {
    public:

        /**
         * 获取path目录下符合subifx后缀的文件，存储到files中
         *      files 存储path目录下的文件(存储的是绝对路径)
         *      path 目录(路径)
         *      subifx 要求文件的后缀
        */
        static void ListAllFile(std::vector<std::string>& files
                                ,const std::string& path
                                ,const std::string& subfix);
                                
        static bool Mkdir(const std::string& dirname);  //创建目录
        
        static bool IsRunningPidfile(const std::string& pidfile);   //判断进程pid文件是否存在且正在运行
        // static bool Rm(const std::string& path);
        // static bool Mv(const std::string& from, const std::string& to);
        // static bool Realpath(const std::string& path, std::string& rpath);
        // static bool Symlink(const std::string& frm, const std::string& to);
        // static bool Unlink(const std::string& filename, bool exist = false);
        // static std::string Dirname(const std::string& filename);
        // static std::string Basename(const std::string& filename);
        // static bool OpenForRead(std::ifstream& ifs, const std::string& filename
        //                 ,std::ios_base::openmode mode);
        // static bool OpenForWrite(std::ofstream& ofs, const std::string& filename
        //                 ,std::ios_base::openmode mode);
    };

    /**
     * @brief 从m中获取k对应的参数,并转换成V类型的
     * @return 返回v类型值
    */
    template<class V, class Map, class K>
    V GetParamValue(const Map& m, const K& k, const V& def = V()) 
    {
        auto it = m.find(k);
        if(it == m.end()) 
        {
            return def;
        }
        try 
        {
            return boost::lexical_cast<V>(it->second);  //实现字符串与目标类型之间的转换
        } catch (...) 
        {}
        return def;
    }

    /**
     * @brief 从m中获取k对应的参数,并转换成V类型的,存储到v中
     * @return 返回是否成功
    */
    template<class V, class Map, class K>
    bool CheckGetParamValue(const Map& m, const K& k, V& v) 
    {
        auto it = m.find(k);
        if(it == m.end()) 
        {
            return false;
        }
        try 
        {
            v = boost::lexical_cast<V>(it->second);
            return true;
        } catch (...) 
        {}
        return false;
    }

    /**
     * @brief 字符串类型转换
    */
    class TypeUtil
    {
    public:
        /**
         * @brief 字符串转换为字符
        */
        static int8_t ToChar(const std::string& str);
        /**
         * @brief 字符串转换为正整数
        */
        static int64_t Atoi(const std::string& str);
        /**
         * @brief 字符串转换成浮点数
        */
        static double Atof(const std::string& str);
        /**
         * @brief 字符串转换为字符
        */
        static int8_t ToChar(const char* str);
        /**
         * @brief 字符串转换为正整数
        */
        static int64_t Atoi(const char* str);
        /**
         * @brief 字符串转换成浮点数
        */
        static double Atof(const char* str);
    };

    /**
     * @brief __sync_系列函数封装
    */
    class Atomic
    {
    public:

        /**
         * @brief 将value加到*ptr上，结果更新到*ptr，并返回操作之后新*ptr的值
        */
        template<class T, class S = T>
        static T addFetch(volatile T& t, S v = 1) 
        {
            return __sync_add_and_fetch(&t, (T)v);
        }
        /**
         * @brief 从*ptr减去value，结果更新到*ptr，并返回操作之后新*ptr的值
        */
        template<class T, class S = T>
        static T subFetch(volatile T& t, S v = 1) 
        {
            return __sync_sub_and_fetch(&t, (T)v);
        }
        /**
         * @brief 将*ptr与value相或， 结果更新到*ptr，并返回操作之后新*ptr的值
        */
        template<class T, class S>
        static T orFetch(volatile T& t, S v) 
        {
            return __sync_or_and_fetch(&t, (T)v);
        }
        /**
         * @brief 将*ptr与value相与，结果更新到*ptr，并返回操作之后新*ptr的值
        */
        template<class T, class S>
        static T andFetch(volatile T& t, S v) 
        {
            return __sync_and_and_fetch(&t, (T)v);
        }
        /**
         * @brief 将*ptr与value异或，结果更新到*ptr，并返回操作之后新*ptr的值
        */
        template<class T, class S>
        static T xorFetch(volatile T& t, S v) 
        {
            return __sync_xor_and_fetch(&t, (T)v);
        }
        /**
         * @brief 将*ptr取反后，与value相与，结果更新到*ptr，并返回操作之后新*ptr的值
        */
        template<class T, class S>
        static T nandFetch(volatile T& t, S v) 
        {
            return __sync_nand_and_fetch(&t, (T)v);
        }
        /**
         * @brief 将value加到*ptr上，结果更新到*ptr，并返回操作之前*ptr的值
        */
        template<class T, class S>
        static T fetchAdd(volatile T& t, S v = 1) 
        {
            return __sync_fetch_and_add(&t, (T)v);
        }
        /**
         * @brief 从*ptr减去value，结果更新到*ptr，并返回操作之前*ptr的值
        */
        template<class T, class S>
        static T fetchSub(volatile T& t, S v = 1) 
        {
            return __sync_fetch_and_sub(&t, (T)v);
        }
        /**
         * @brief 将*ptr与value相或，结果更新到*ptr， 并返回操作之前*ptr的值
        */
        template<class T, class S>
        static T fetchOr(volatile T& t, S v) 
        {
            return __sync_fetch_and_or(&t, (T)v);
        }
        /**
         * @brief 将*ptr与value相与，结果更新到*ptr，并返回操作之前*ptr的值
        */
        template<class T, class S>
        static T fetchAnd(volatile T& t, S v) 
        {
            return __sync_fetch_and_and(&t, (T)v);
        }
        /**
         * @brief 将*ptr与value异或，结果更新到*ptr，并返回操作之前*ptr的值
        */
        template<class T, class S>
        static T fetchXor(volatile T& t, S v) 
        {
            return __sync_fetch_and_xor(&t, (T)v);
        }
        /**
         * @brief 将*ptr取反后，与value相与，结果更新到*ptr，并返回操作之前*ptr的值
        */
        template<class T, class S>
        static T fetchNand(volatile T& t, S v) {
            return __sync_fetch_and_nand(&t, (T)v);
        }
        /**
         * @brief 比较*ptr与oldval的值，如果两者相等，则将newval更新到*ptr并返回操作之前*ptr的值
        */
        template<class T, class S>
        static T compareAndSwap(volatile T& t, S old_val, S new_val) 
        {
            return __sync_val_compare_and_swap(&t, (T)old_val, (T)new_val);
        }

        /**
         * @brief 比较*ptr与oldval的值，如果两者相等，则将newval更新到*ptr并返回true
        */
        template<class T, class S>
        static bool compareAndSwapBool(volatile T& t, S old_val, S new_val) 
        {
            return __sync_bool_compare_and_swap(&t, (T)old_val, (T)new_val);    //比较参数t是否等于参数old_val, 等于就设置为1, 返回新值; 不等于就直接返回旧值.
        }
    };

    /**
     * @brief 释放各种类型的数组
    */
    template<class T>
    void delete_array(T* v) 
    {
        if(v) 
        {
            delete[] v;
        }
    }

    /**
     * @brief 获取主机名称
    */
    std::string GetHostName();
    /**
     * @brief 获取主机IPv4
    */
    std::string GetIPv4();

    /**
     * @brief l流式输出begin到end的数组,并使用tag字符串隔开
    */
    template<class Iter>
    std::string Join(Iter begin, Iter end, const std::string& tag)
    {
        std::stringstream ss;
        for(Iter it = begin; it != end; ++it) 
        {
            if(it != begin) 
            {
                ss << tag;
            }
            ss << *it;
        }
        return ss.str();
    }

}

#endif