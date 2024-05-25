#include"http_session.h"
#include"http_parser.h"

namespace sylar
{
    namespace http
    {

        HttpSession::HttpSession(Socket::ptr sock, bool owner)  //给父类的成员函数初始化
            :SocketStream(sock, owner)
        {}

        HttpRequest::ptr HttpSession::recvRequest() //接收HTTP请求,并解析HTTP请求
        {
            HttpRequestParser::ptr parser(new HttpRequestParser);   //HTTP请求解析类
            uint64_t buff_size = HttpRequestParser::GetHttpRequestBufferSize(); //返回HttpRequest协议解析的缓存大小(限制请求头部一个字段(一次发送的数据大小))
            //uint64_t buff_size = 100;
            std::shared_ptr<char> buffer(   //共享字符串
                    new char[buff_size], [](char* ptr){
                        delete[] ptr;   //析构函数(自动释放)
                    });
            char* data = buffer.get();  //存储HTTP请求协议
            int offset = 0; //上一轮 已读取的数据中还未解析的长度(消息体==消息体不用解析)
            do {
                int len = read(data + offset, buff_size - offset);  //读取HTTP请求协议  (返回实际接收到的数据长度)
                if(len <= 0)
                {
                    close();
                    return nullptr;
                }
                len += offset;  //现在 已读取的数据中还未解析的长度(消息体==消息体不用解析) <=buff_size
                size_t nparse = parser->execute(data, len); //解析Http请求协议
                if(parser->hasError())
                {
                    close();
                    return nullptr;
                }
                offset = len - nparse;  //这一轮之后 已读取的数据中还未解析的长度(消息体)
                if(offset == (int)buff_size)    //出错(没有解析到) nparse==0
                {
                    close();
                    return nullptr;
                }
                if(parser->isFinished())    //解析完成
                {
                    break;
                }
            } while(true);
            int64_t length = parser->getContentLength();    //获取消息体长度
            if(length > 0)
            {
                std::string body;   //存储消息体(请求协议)
                body.resize(length);

                int len = 0;    //实际消息体
                if(length >= offset)    //消息体长度 > 已读取的数据中还未解析的长度(消息体)
                {
                    memcpy(&body[0], data, offset);
                    len = offset;
                }
                else
                {
                    memcpy(&body[0], data, length);
                    len = length;
                }
                length -= offset;   //还未读取的消息体数据
                if(length > 0)
                {
                    if(readFixSize(&body[len], length) <= 0)    //读取剩下的消息体数据
                    {
                        close();
                        return nullptr;
                    }
                }
                parser->getData()->setBody(body);   //设置消息体
            }

            //parser->getData()->init();
            return parser->getData();   //返回HttpRequest结构(HTTP请求结构体)
        }

        int HttpSession::sendResponse(HttpResponse::ptr rsp)    //发送HTTP响应
        {
            std::stringstream ss;
            ss << *rsp;
            std::string data = ss.str();
            return writeFixSize(data.c_str(), data.size()); //发送HTTP响应
        }

    }
}