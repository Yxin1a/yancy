//常用宏的封装
#ifndef __SYLAR_MACRO_H__   //防止头文件重复
#define __SYLAR_MACRO_H__

//协程——>线程——>进程
//轻量————————>重量
//协程是用户态的；是我们自己操控切换的
//线程是系统态的；是系统自己切换的
//从切换角度来看；协程要快

#include<string.h>
#include<assert.h>  //可加#define NDEBUG禁用assert函数
#include"log.h"
#include"util.h"

//成功概率大，小，使用可以提高性能
#if defined __GNUC__ || defined __llvm__    //有些编译器执行，有些不执行
/// LIKCLY 宏的封装, 告诉编译器优化,条件大概率成立(成功几率大)
#   define SYLAR_LIKELY(x)       __builtin_expect(!!(x), 1)
/// LIKCLY 宏的封装, 告诉编译器优化,条件大概率不成立(成功几率小)
#   define SYLAR_UNLIKELY(x)     __builtin_expect(!!(x), 0)
#else
#   define SYLAR_LIKELY(x)      (x)
#   define SYLAR_UNLIKELY(x)      (x)
#endif

/*  assert()
    功能：如果形参为假则终止程序(已放弃 (核心已转储))
    注意：每个assert只能检查一个条件，如果多个条件不好判断是哪个条件的错误
    具体：如果形参数为假，assert 向 stderr打印一条出错信息，
         信息包含文件名、表达式、行号，然后调用abort终止程序
         如果形参为真，程序继续执行
    优点：可以方便我们进行程序调试，同时对于绝对不能出错（条件为假）的地方使用可以有效的预防出现更多的错误
    缺点：assert是宏函数，频繁的调用会增加额外的开销，影响程序性能
*/

//assert()生成的错误信息在core文件中，要在vscode的gdb调试查看
//断言(逻辑判断式)宏封装——当程序执行到断言的位置时，对应的断言应该为真。若断言不为真时，程序会中止执行，并给出错误信息。
#define SYLAR_ASSERT(x) \
    if(SYLAR_UNLIKELY(!(x))) \
    { \
        SYLAR_LOG_ERROR(SYLAR_LOG_ROOT())<<"ASSERTION:"#x \
            <<"\nbacktrace:\n" \
            <<sylar::BacktraceToString(100,2,"   "); \
        assert(x); \
    }

//断言(逻辑判断式)宏封装——当程序执行到断言的位置时，对应的断言应该为真。若断言不为真时，程序会中止执行，并给出错误信息。
#define SYLAR_ASSERT2(x,w) \
    if(SYLAR_UNLIKELY(!(x))) \
    { \
        SYLAR_LOG_ERROR(SYLAR_LOG_ROOT())<<"ASSERTION:"#x \
            <<"\n"<<w \
            <<"\nbacktrace:\n" \
            <<sylar::BacktraceToString(100,2,"   "); \
        assert(x); \
    }

#endif