//  字节序操作函数(大端/小端)
//网络字节序<————>主机字节序
//大端<————>小端
//1字节 大端和小端都是一样
#ifndef __SYLAR_ENDIAN_H__
#define __SYLAR_ENDIAN_H__

#define SYLAR_LITTLE_ENDIAN 1   //小端
#define SYLAR_BIG_ENDIAN 2  //大端

#include<byteswap.h>    //转换
#include<stdint.h>
#include<type_traits>   //enable_if

namespace sylar
{

    /**
     *  8字节类型的字节序转化
     */
    template<class T>
    typename std::enable_if<sizeof(T) == sizeof(uint64_t), T>::type
    byteswap(T value)
    {
        return (T)bswap_64((uint64_t)value);    //8字节类型的字节序转化
    }

    /**
     *  4字节类型的字节序转化
     */
    template<class T>
    typename std::enable_if<sizeof(T) == sizeof(uint32_t), T>::type
    byteswap(T value)
    {
        return (T)bswap_32((uint32_t)value);    //4字节类型的字节序转化
    }

    /**
     *  2字节类型的字节序转化
     */
    template<class T>
    typename std::enable_if<sizeof(T) == sizeof(uint16_t), T>::type
    byteswap(T value)
    {
        return (T)bswap_16((uint16_t)value);    //2字节类型的字节序转化
    }

    #if BYTE_ORDER == BIG_ENDIAN    //判断是不是大端
    #define SYLAR_BYTE_ORDER SYLAR_BIG_ENDIAN   //是大端
    #else
    #define SYLAR_BYTE_ORDER SYLAR_LITTLE_ENDIAN    //是小端
    #endif


    #if SYLAR_BYTE_ORDER == SYLAR_BIG_ENDIAN   //如果是大端 

    /**
     *                  字节序转换
     *  只在小端机器上执行byteswap, 在大端机器上什么都不做
     * (一端执行转换，另一端默认执行)
     */
    template<class T>
    T byteswapOnLittleEndian(T t)
    {
        return t;
    }

    /**
     *                  字节序转换
     *  只在大端机器上执行byteswap, 在小端机器上什么都不做
     * (一端执行转换，另一端默认执行)
     */
    template<class T>
    T byteswapOnBigEndian(T t)
    {
        return byteswap(t);
    }

    #else

    /**
     *                  字节序转换
     *  只在小端机器上执行byteswap, 在大端机器上什么都不做
     * (一端执行转换，另一端默认执行)
     */
    template<class T>
    T byteswapOnLittleEndian(T t)
    {
        return byteswap(t);
    }

    /**
     *                  字节序转换
     *  只在大端机器上执行byteswap, 在小端机器上什么都不做
     * (一端执行转换，另一端默认执行)
     */
    template<class T>
    T byteswapOnBigEndian(T t)
    {
        return t;
    }
    #endif
}

#endif