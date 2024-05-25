#include"bytearray.h"
#include <fstream>
#include <sstream>
#include <string.h>
#include <iomanip>

#include "endian.h"
#include"log.h"

namespace sylar
{

    static sylar::Logger::ptr g_logger=SYLAR_LOG_NAME("system");

    //bytearray
    //Node
    ByteArray::Node::Node(size_t s) //构造指定大小的内存块  内存块初始化
        :ptr(new char[s])
        ,next(nullptr)
        ,size(s)
    {}

    ByteArray::Node::Node() //内存块初始化
        :ptr(nullptr)
        ,next(nullptr)
        ,size(0)
    {}

    ByteArray::Node::~Node()    //释放内存
    {
        if(ptr)
        {
            delete[] ptr;
        }
    }

    ByteArray::ByteArray(size_t base_size)  //使用指定长度的内存块构造ByteArray 初始化
        :m_baseSize(base_size)
        ,m_position(0)
        ,m_capacity(base_size)
        ,m_size(0)
        ,m_endian(SYLAR_BIG_ENDIAN) //大端
        ,m_root(new Node(base_size))
        ,m_cur(m_root)
    {}

    ByteArray::~ByteArray() //释放链表内存
    {
        Node* tmp = m_root;
        while(tmp)
        {
            m_cur = tmp;
            tmp = tmp->next;
            delete m_cur;
        }
    }


    //write
    //普通类型(char),(short),(int,long,float),(double,longlong)(固定长度)

    void ByteArray::writeFint8  (int8_t value)  //写入固定长度int8_t类型的数据
    {
        write(&value, sizeof(value));   //写入内存池
    }

    void ByteArray::writeFuint8 (uint8_t value) //写入固定长度uint8_t类型的数据
    {
        write(&value, sizeof(value));   //写入内存池
    }

    void ByteArray::writeFint16 (int16_t value) //写入固定长度int16_t类型的数据(大端/小端)
    {
        if(m_endian != SYLAR_BYTE_ORDER)    //不是小端
        {
            value = byteswap(value);    //字节序转化
        }
        write(&value, sizeof(value));   //写入内存池
    }

    void ByteArray::writeFuint16(uint16_t value)    //写入固定长度uint16_t类型的数据(大端/小端)
    {
        if(m_endian != SYLAR_BYTE_ORDER)    //不是小端
        {
            value = byteswap(value);    //字节序转化
        }
        write(&value, sizeof(value));   //写入内存池
    }

    void ByteArray::writeFint32 (int32_t value)
    {
        if(m_endian != SYLAR_BYTE_ORDER)    //不是小端
        {
            value = byteswap(value);    //字节序转化
        }
        write(&value, sizeof(value));   //写入内存池
    }

    void ByteArray::writeFuint32(uint32_t value)
    {
        if(m_endian != SYLAR_BYTE_ORDER)    //不是小端
        {
            value = byteswap(value);    //字节序转化
        }
        write(&value, sizeof(value));   //写入内存池
    }

    void ByteArray::writeFint64 (int64_t value)
    {
        if(m_endian != SYLAR_BYTE_ORDER)    //不是小端
        {
            value = byteswap(value);    //字节序转化
        }
        write(&value, sizeof(value));   //写入内存池
    }

    void ByteArray::writeFuint64(uint64_t value)
    {
        if(m_endian != SYLAR_BYTE_ORDER)    //不是小端
        {
            value = byteswap(value);    //字节序转化
        }
        write(&value, sizeof(value));   //写入内存池
    }

    //普通类型int,long,longlong(可变长度==压缩)

    static uint32_t EncodeZigzag32(const int32_t& v)    //有符号int32_t转化成无符号(写入压缩)
    {
        if(v < 0)   //负数
        {
            return ((uint32_t)(-v)) * 2 - 1;
        }
        else
        {
            return v * 2;
        }
    }

    static uint64_t EncodeZigzag64(const int64_t& v)    //有符号int64_t转化成无符号(写入压缩)
    {
        if(v < 0)   //负数
        {
            return ((uint64_t)(-v)) * 2 - 1;
        }
        else
        {
            return v * 2;
        }
    }

    static int32_t DecodeZigzag32(const uint32_t& v)    //无符号int32_t转化成有符号(读取压缩)
    {
        return (v >> 1) ^ -(v & 1);
    }

    static int64_t DecodeZigzag64(const uint64_t& v)    //无符号int64_t转化成有符号(读取压缩)
    {
        return (v >> 1) ^ -(v & 1);
    }

    void ByteArray::writeInt32  (int32_t value)
    {
        writeUint32(EncodeZigzag32(value));
    }

    void ByteArray::writeUint32 (uint32_t value)    //写入无符号Varint32类型的数据(压缩1 ~ 5字节)
    {
        uint8_t tmp[5]; //最高5个字节(倒着存)
        uint8_t i = 0;
        while(value >= 0x80)    //0x80 --> 128 (1000 0000)  *谷歌压缩用每一字节最高位为1则还有后续数据(存的压缩数据表示)
        {   //压缩
            tmp[i++] = (value & 0x7F) | 0x80;   //0x7F --> 127 (0111 1111)
            value >>= 7;    //右移7位(右位移y位，正数头部用0补上，对于负数，头部用1补上)
        }
        tmp[i++] = value;   //将最后一段字节存进来
        write(tmp, i);
    }

    void ByteArray::writeInt64  (int64_t value)
    {
        writeUint64(EncodeZigzag64(value));
    }

    void ByteArray::writeUint64 (uint64_t value)    //写入无符号Varint64类型的数据(压缩1 ~ 10字节)
    {
        uint8_t tmp[10];    //最高10个字节(倒着存)
        uint8_t i = 0;
        while(value >= 0x80)    //0x80 --> 128 (1000 0000)  *谷歌压缩用每一字节最高位为1则还有后续数据(存的压缩数据表示)
        {   //压缩
            tmp[i++] = (value & 0x7F) | 0x80;   //0x7F --> 127 (0111 1111)存一个字节
            value >>= 7;    //右移7位(右位移y位，正数头部用0补上，对于负数，头部用1补上)
        }
        tmp[i++] = value;   //将最后一段字节存进来
        write(tmp, i);
    }

    //普通类型float，double,string
    
    void ByteArray::writeFloat  (float value)   //写入float类型的数据   float——>uint32_t
    {
        uint32_t v;
        memcpy(&v, &value, sizeof(value));  //将float存到uint32_t类型中
        writeFuint32(v);
    }

    void ByteArray::writeDouble (double value)  //写入double类型的数据  double——>uint64_t
    {
        uint64_t v;
        memcpy(&v, &value, sizeof(value));  //将double存到uint64_t类型中
        writeFuint64(v);
    }

    void ByteArray::writeStringF16(const std::string& value)    //写入std::string类型的数据,用uint16_t作为长度类型
    {
        writeFuint16(value.size()); //先写入长度
        write(value.c_str(), value.size()); //后写入数据
    }

    void ByteArray::writeStringF32(const std::string& value)    //写入std::string类型的数据,用uint32_t作为长度类型
    {
        writeFuint32(value.size()); //先写入长度
        write(value.c_str(), value.size()); //后写入数据
    }

    void ByteArray::writeStringF64(const std::string& value)    //写入std::string类型的数据,用uint64_t作为长度类型
    {
        writeFuint64(value.size()); //先写入长度
        write(value.c_str(), value.size()); //后写入数据
    }

    void ByteArray::writeStringVint(const std::string& value)   //写入std::string类型的数据,用无符号Varint64作为长度类型(可变长度)
    {
        writeUint64(value.size());  //先写入长度
        write(value.c_str(), value.size()); //后写入数据
    }

    void ByteArray::writeStringWithoutLength(const std::string& value)  //写入std::string类型的数据,无长度
    {
        write(value.c_str(), value.size()); //写入数据
    }


    //read ( getReadSize()可读取的数据大小 )

    //普通类型char,short,int,long,longlong(固定长度)

    int8_t   ByteArray::readFint8() //读取int8_t类型的数据
    {
        int8_t v;   //存数据
        read(&v, sizeof(v));
        return v;
    }

    uint8_t  ByteArray::readFuint8()    //读取uint8_t类型的数据
    {
        uint8_t v;   //存数据
        read(&v, sizeof(v));
        return v;
    }

//大端小端字节序转换，读取数据
#define XX(type) \
    type v; \
    read(&v, sizeof(v)); \
    if(m_endian == SYLAR_BYTE_ORDER)    \
    { \
        return v; \
    }   \
    else    \
    { \
        return byteswap(v); \
    }

    int16_t  ByteArray::readFint16()    //读取int16_t类型的数据
    {
        XX(int16_t);
    }

    uint16_t ByteArray::readFuint16()   //读取uint16_t类型的数据
    {
        XX(uint16_t);
    }

    int32_t  ByteArray::readFint32()    //读取int32_t类型的数据
    {
        XX(int32_t);
    }

    uint32_t ByteArray::readFuint32()   //读取uint32_t类型的数据
    {
        XX(uint32_t);
    }

    int64_t  ByteArray::readFint64()    //读取int64_t类型的数据
    {
        XX(int64_t);
    }

    uint64_t ByteArray::readFuint64()   //读取uint64_t类型的数据
    {
        XX(uint64_t);
    }

#undef XX

    //普通类型int,long,longlong(可变长度)

    int32_t  ByteArray::readInt32() //读取有符号Varint32类型的数据
    {
        return DecodeZigzag32(readUint32());
    }

    uint32_t ByteArray::readUint32()    //读取无符号Varint32类型的数据
    {
        uint32_t result = 0;
        for(int i = 0; i < 32; i += 7)
        {
            uint8_t b = readFuint8();   //每次取一个字节来看
            if(b < 0x80)    //是不是读取数据的最后一个节点  1个字节最高位是1，还有数据
            {
                result |= ((uint32_t)b) << i;   //左移补零
                break;
            }
            else
            {
                result |= (((uint32_t)(b & 0x7f)) << i);    //左移补零  ，把最高位1变成零
            }
        }
        return result;
    }

    int64_t  ByteArray::readInt64() //读取有符号Varint64类型的数据
    {
        return DecodeZigzag64(readUint64());
    }

    uint64_t ByteArray::readUint64()    //读取无符号Varint64类型的数据
    {
        uint64_t result = 0;
        for(int i = 0; i < 64; i += 7)
        {
            uint8_t b = readFuint8();   //每次取一个字节来看
            if(b < 0x80)    //是不是读取数据的最后一个节点  1个字节最高位是1，还有数据
            {
                result |= ((uint64_t)b) << i;   //左移补零
                break;
            }
            else
            {
                result |= (((uint64_t)(b & 0x7f)) << i);    //左移补零  ，把最高位1变成零
            }
        }
        return result;
    }

    //普通类型float，double,string

    float    ByteArray::readFloat() //读取float类型的数据
    {
        uint32_t v = readFuint32(); //uint32_t <==> float
        float value;
        memcpy(&value, &v, sizeof(v));
        return value;
    }

    double   ByteArray::readDouble()    //读取double类型的数据
    {
        uint64_t v = readFuint64(); //uint64_t <==> double
        double value;
        memcpy(&value, &v, sizeof(v));
        return value;
    }

    std::string ByteArray::readStringF16()  //读取std::string类型的数据,用uint16_t作为长度
    {
        uint16_t len = readFuint16();   //读取长度
        std::string buff;
        buff.resize(len);
        read(&buff[0], len);    //读取数据
        return buff;
    }

    std::string ByteArray::readStringF32()  //读取std::string类型的数据,用uint32_t作为长度
    {
        uint32_t len = readFuint32();   //读取长度
        std::string buff;
        buff.resize(len);
        read(&buff[0], len);    //读取数据
        return buff;
    }

    std::string ByteArray::readStringF64()  //读取std::string类型的数据,用uint64_t作为长度
    {
        uint64_t len = readFuint64();   //读取长度
        std::string buff;
        buff.resize(len);
        read(&buff[0], len);    //读取数据
        return buff;
    }

    std::string ByteArray::readStringVint() //读取std::string类型的数据,用无符号Varint64作为长度
    {
        uint64_t len = readUint64();    //读取长度
        std::string buff;
        buff.resize(len);
        read(&buff[0], len);    //读取数据
        return buff;
    }

    //内部操作

    void ByteArray::clear() //清空ByteArray(数据)
    {
        m_position = m_size = 0;
        m_capacity = m_baseSize;    //除了第一块，全部释放
        Node* tmp = m_root->next;
        while(tmp)
        {
            m_cur = tmp;
            tmp = tmp->next;
            delete m_cur;
        }
        m_cur = m_root;
        m_root->next = NULL;
    }

    void ByteArray::write(const void* buf, size_t size) //写入size长度的数据buf到内存池
    {
        if(size == 0)
        {
            return;
        }
        addCapacity(size);  //扩容ByteArray,使其可以容纳size个数据

        size_t npos = m_position % m_baseSize;  //最后一个节点已存了多少
        size_t ncap = m_cur->size - npos;   //最后一个节点所剩余的大小
        size_t bpos = 0;    //buf数据当前已存了多少

        while(size > 0)
        {
            if(ncap >= size)    //最后一个节点所剩余的大小 > size
            {
                memcpy(m_cur->ptr + npos, (const char*)buf + bpos, size);
                if(m_cur->size == (npos + size))    //刚刚好存够一个节点
                {
                    m_cur = m_cur->next;
                }
                m_position += size;
                bpos += size;
                size = 0;
            }
            else
            {
                memcpy(m_cur->ptr + npos, (const char*)buf + bpos, ncap);
                m_position += ncap;
                bpos += ncap;   //buf数据当前已存了多少
                size -= ncap;   //剩余多少没有存进
                m_cur = m_cur->next;    //不够，新创一个节点
                ncap = m_cur->size;
                npos = 0;
            }
        }

        if(m_position > m_size)
        {
            m_size = m_position;
        }
    }

    void ByteArray::read(void* buf, size_t size)    //从内存池读取size长度的数据buf(从头读到尾)
    {
        if(size > getReadSize())    //可读取数据大小 < 要读取数据
        {
            throw std::out_of_range("not enough len");
        }

        size_t npos = m_position % m_baseSize;  //第一个节点已读取多少
        size_t ncap = m_cur->size - npos;   //第一个节点所剩余多少数据还没读取
        size_t bpos = 0;    //buf内存已存进了多少
        while(size > 0)
        {
            if(ncap >= size)    //第一个节点所剩余多少数据还没读取 > size
            {
                memcpy((char*)buf + bpos, m_cur->ptr + npos, size);
                if(m_cur->size == (npos + size))    //刚刚好读取够一个节点
                {
                    m_cur = m_cur->next;
                }
                m_position += size;
                bpos += size;
                size = 0;
            }
            else
            {
                memcpy((char*)buf + bpos, m_cur->ptr + npos, ncap);
                m_position += ncap;
                bpos += ncap;   //buf内存已存进了多少
                size -= ncap;
                m_cur = m_cur->next;    //不够，下一个节点
                ncap = m_cur->size;
                npos = 0;
            }
        }
    }

    void ByteArray::read(void* buf, size_t size, size_t position) const //从内存池中position位置读取size长度的数据(不影响当前操作的位置)
    {
        if(size > (m_size - position))  //可读取数据大小 < 要读取数据
        {
            throw std::out_of_range("not enough len");
        }

        size_t npos = position % m_baseSize;    //第一个节点已读取多少  (从position位置开始)
        //size_t ncap = m_cur->size - npos;   //第一个节点所剩余多少数据还没读取  (从position位置开始)
        size_t bpos = 0;    //buf内存已存进了多少
        size_t count = position / m_baseSize;   //前面已被读取的节点个数
        //Node* cur = m_cur;  //保存原来的操作内存块指针
        Node* cur = m_root; //第一个内存块指针
        while(count > 0)    //寻找position对应的节点指针
        {
            cur = cur->next;
            --count;
        }

        size_t ncap = cur->size - npos; //第一个节点所剩余多少数据还没读取  (从position位置开始)

        while(size > 0)
        {
            if(ncap >= size)    //第一个节点所剩余多少数据还没读取 > size
            {
                memcpy((char*)buf + bpos, cur->ptr + npos, size);
                if(cur->size == (npos + size))  //刚刚好读取够一个节点
                {
                    cur = cur->next;
                }
                position += size;
                bpos += size;
                size = 0;
            }
            else
            {
                memcpy((char*)buf + bpos, cur->ptr + npos, ncap);
                position += ncap;
                bpos += ncap;   //buf内存已存进了多少
                size -= ncap;
                cur = cur->next;    //不够，下一个节点
                ncap = cur->size;
                npos = 0;
            }
        }
    }

    void ByteArray::setPosition(size_t v) //设置ByteArray当前位置
    {
        if(v > m_capacity)  //当前的总容量 < ByteArray当前位置
        {
            throw std::out_of_range("set_position out of range");
        }
        
        m_position = v;
        
        if(m_position > m_size) //当前操作位置 > 当前数据的大小
        {
            m_size = m_position;
        }

        m_cur = m_root; //重新开始设置当前操作的内存块指针
        while(v > m_cur->size)  //设置当前操作的内存块指针
        {
            v -= m_cur->size;
            m_cur = m_cur->next;
        }
        if(v == m_cur->size)
        {
            m_cur = m_cur->next;
        }
    }

    bool ByteArray::writeToFile(const std::string& name) const  //把ByteArray的当前所有数据写入到name文件中(不影响当前操作的位置)
    {
        std::ofstream ofs;
        ofs.open(name, std::ios::trunc | std::ios::binary); //trunc如果文件存在先删除再创建     binary二进制方式
        if(!ofs)    //打开失败
        {
            SYLAR_LOG_ERROR(g_logger) << "writeToFile name=" << name
                << " error , errno=" << errno << " errstr=" << strerror(errno);
            return false;
        }

        int64_t read_size = getReadSize();  //可读取数据大小
        int64_t pos = m_position;   //当前操作位置
        Node* cur = m_cur;  //保存原来的操作内存块指针

        while(read_size > 0)
        {
            int diff = pos % m_baseSize;    //第一个节点已读取多少
            int64_t len = (read_size > (int64_t)m_baseSize ? m_baseSize : read_size) - diff;    //一次写入文件的长度
            ofs.write(cur->ptr + diff, len);
            cur = cur->next;
            pos += len;
            read_size -= len;
        }

        return true;
    }

    bool ByteArray::readFromFile(const std::string& name)   //从name文件中读取数据
    {
        std::ifstream ifs;
        ifs.open(name, std::ios::binary);   //binary二进制方式
        if(!ifs)    //打开失败
        {
            SYLAR_LOG_ERROR(g_logger) << "readFromFile name=" << name
                << " error, errno=" << errno << " errstr=" << strerror(errno);
            return false;
        }

        std::shared_ptr<char> buff(new char[m_baseSize], /*析构函数*/[](char* ptr) { delete[] ptr;});       //临时存储，用完自动释放
        while(!ifs.eof())   //eof()文件结束
        {
            ifs.read(buff.get(), m_baseSize);   //读取文件
            write(buff.get(), ifs.gcount());    //写入ifs.gcount()长度的数据到内存池    gcount() read方法读取的字符数
        }
        return true;
    }

    bool ByteArray::isLittleEndian() const    //是否是小端
    {
        return m_endian == SYLAR_LITTLE_ENDIAN; //小端
    }

    void ByteArray::setIsLittleEndian(bool val)   //是否设置为小端
    {
        if(val)
        {
            m_endian = SYLAR_LITTLE_ENDIAN; //小端
        }
        else
        {
            m_endian = SYLAR_BIG_ENDIAN;    //大端
        }
    }

    std::string ByteArray::toString() const   //将ByteArray里面的数据[m_position, m_size)转成std::string
    {
        std::string str;
        str.resize(getReadSize());  //可读取数据大小
        if(str.empty())
        {
            return str;
        }
        read(&str[0], str.size(), m_position);  //读取数据到字符串中
        return str;
    }

    std::string ByteArray::toHexString() const    //将ByteArray里面的数据[m_position, m_size)转成16进制的std::string(格式:FF FF FF)
    {
        std::string str = toString();   //将ByteArray里面的数据[m_position, m_size)转成std::string
        std::stringstream ss;

        for(size_t i = 0; i < str.size(); ++i)
        {
            if(i > 0 && i % 32 == 0)    //每32个字节就换行
            {
                ss << std::endl;
            }
            ss << std::setw(2) << std::setfill('0') << std::hex //setw控制输出之间的间隔(字节——默认空格)  setfill指定setw的符号填充
            << (int)(uint8_t)str[i] << " "; //hex十六进制   (int)(uint8_t)str[i]
        }

        return ss.str();
    }

    uint64_t ByteArray::getReadBuffers(std::vector<iovec>& buffers, uint64_t len) const //获取可读取的缓存,保存成iovec数组(不影响当前操作的位置)
    {
        //获取可读取的缓存
        len = len > getReadSize() ? getReadSize() : len;    //getReadSize可读取数据大小
        if(len == 0)
        {
            return 0;
        }

        uint64_t size = len;

        size_t npos = m_position % m_baseSize;  //第一个节点已读取多少
        size_t ncap = m_cur->size - npos;   //第一个节点所剩余多少数据还没读取
        struct iovec iov;   //iov_base 指向一个缓冲区，这个缓冲区存放的是网络接收的数据（read），或者网络将要发送的数据（write）。
                            //iovec 结构体的字段 iov_len 存放的是接收数据的最大长度（read），或者实际写入的数据长度（write）
        Node* cur = m_cur;  //保存原来的操作内存块指针

        while(len > 0)
        {
            if(ncap >= len) //第一个节点所剩余多少数据还没读取 > len
            {
                iov.iov_base = cur->ptr + npos; //指向数据指针
                iov.iov_len = len;  //数据长度
                len = 0;
            }
            else
            {
                iov.iov_base = cur->ptr + npos; //指向数据指针
                iov.iov_len = ncap; //数据长度
                len -= ncap;
                cur = cur->next;
                ncap = cur->size;
                npos = 0;
            }
            buffers.push_back(iov); //iovec数组的插入
        }
        return size;
    }

    uint64_t ByteArray::getReadBuffers(std::vector<iovec>& buffers, uint64_t len, uint64_t position) const  //获取可读取的缓存,保存成iovec数组,从position位置开始(不影响当前操作的位置)
    {
        //获取可读取的缓存
        len = len > getReadSize() ? getReadSize() : len;    //getReadSize可读取数据大小
        if(len == 0)
        {
            return 0;
        }

        uint64_t size = len;

        size_t npos = position % m_baseSize;    //第一个节点已读取多少
        size_t count = position / m_baseSize;   //前面已被读取的节点个数
        Node* cur = m_root; //第一个内存块指针
        while(count > 0)    //寻找position对应的节点指针
        {
            cur = cur->next;
            --count;
        }

        size_t ncap = cur->size - npos; //第一个节点所剩余多少数据还没读取  (从position位置开始)
        struct iovec iov;   //iov_base 指向一个缓冲区，这个缓冲区存放的是网络接收的数据（read），或者网络将要发送的数据（write）。
                            //iovec 结构体的字段 iov_len 存放的是接收数据的最大长度（read），或者实际写入的数据长度（write）
        while(len > 0)
        {
            if(ncap >= len) //第一个节点所剩余多少数据还没读取 > len
            {
                iov.iov_base = cur->ptr + npos; //指向数据指针
                iov.iov_len = len;  //数据长度
                len = 0;
            }
            else
            {
                iov.iov_base = cur->ptr + npos; //指向数据指针
                iov.iov_len = ncap; //数据长度
                len -= ncap;
                cur = cur->next;
                ncap = cur->size;
                npos = 0;
            }
            buffers.push_back(iov); //iovec数组的插入
        }
        return size;
    }

    uint64_t ByteArray::getWriteBuffers(std::vector<iovec>& buffers, uint64_t len)  //获取可写入的缓存,保存成iovec数组(不影响当前操作的位置)
    {
        if(len == 0)
        {
            return 0;
        }
        addCapacity(len);   //扩容ByteArray,使其可以容纳len个数据
        uint64_t size = len;

        size_t npos = m_position % m_baseSize;  //最后一个节点已存了多少
        size_t ncap = m_cur->size - npos;   //最后一个节点所剩余的大小
        struct iovec iov;   //iov_base 指向一个缓冲区，这个缓冲区存放的是网络接收的数据（read），或者网络将要发送的数据（write）。
                            //iovec 结构体的字段 iov_len 存放的是接收数据的最大长度（read），或者实际写入的数据长度（write）
        Node* cur = m_cur;  //保存操作的内存块指针
        while(len > 0)
        {
            if(ncap >= len) //最后一个节点所剩余的大小 > len
            {
                iov.iov_base = cur->ptr + npos; //指向数据指针
                iov.iov_len = len;  //数据长度
                len = 0;
            }
            else
            {
                iov.iov_base = cur->ptr + npos; //指向数据指针
                iov.iov_len = ncap; //数据长度

                len -= ncap;
                cur = cur->next;
                ncap = cur->size;
                npos = 0;
            }
            buffers.push_back(iov); //iovec数组的插入
        }
        return size;
    }
    
    void ByteArray::addCapacity(size_t size)  //扩容ByteArray,使其可以容纳size个数据(如果原本可以可以容纳,则不扩容)
    {
        if(size == 0)
        {
            return;
        }
        size_t old_cap = getCapacity(); //当前的可写入容量
        if(old_cap >= size) //够用了
        {
            return;
        }

        size = size - old_cap;  //还要扩容多少
        size_t count = (size / m_baseSize)+(((size % m_baseSize) > 0) ? 1 : 0);   //最少要扩容多少个节点
        
        Node* tmp = m_root;
        while(tmp->next)    //从最后一个节点开始扩容
        {
            tmp = tmp->next;
        }

        Node* first = NULL; //保存原来最后一个节点
        for(size_t i = 0; i < count; ++i)
        {
            tmp->next = new Node(m_baseSize);
            if(first == NULL)
            {
                first = tmp->next;
            }
            tmp = tmp->next;
            m_capacity += m_baseSize;   //当前的总容量
        }

        if(old_cap == 0)
        {
            m_cur = first;
        }
    }
    
}