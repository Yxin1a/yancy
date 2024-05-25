#include "stream.h"

namespace sylar
{
    //虚函数的默认实现
    int Stream::readFixSize(void* buffer, size_t length)    //读固定长度的数据，存储到字符串中
    {
        size_t offset = 0;  //buffer写入的位置
        int64_t left = length;  //读取的剩余内存大小
        while(left > 0)
        {
            int64_t len = read((char*)buffer + offset, left);   //读取数据
            if(len <= 0)    //被关闭、出现流错误
            {
                return len;
            }
            offset += len;
            left -= len;
        }
        return length;
    }

    int Stream::readFixSize(ByteArray::ptr ba, size_t length)   //读固定长度的数据，存储到ByteArray内存池
    {
        int64_t left = length;  //读取的剩余内存大小
        while(left > 0)
        {
            int64_t len = read(ba, left);   //读取数据
            if(len <= 0)    //被关闭、出现流错误
            {
                return len;
            }
            left -= len;
        }
        return length;
    }

    int Stream::writeFixSize(const void* buffer, size_t length) //从字符串中读取数据，写固定长度的数据
    {
        size_t offset = 0;  //buffer读取的位置
        int64_t left = length;  //写入的剩余内存大小
        while(left > 0)
        {
            int64_t len = write((const char*)buffer + offset, left);    //读取数据
            if(len <= 0)    //被关闭、出现流错误
            {
                return len;
            }
            offset += len;
            left -= len;
        }
        return length;

    }

    int Stream::writeFixSize(ByteArray::ptr ba, size_t length)  //从ByteArray内存池中读取数据，写固定长度的数据
    {
        int64_t left = length;  //写入的剩余内存大小
        while(left > 0)
        {
            int64_t len = write(ba, left);  //读取数据
            if(len <= 0)    //被关闭、出现流错误
            {
                return len;
            }
            left -= len;
        }
        return length;
    }

}
