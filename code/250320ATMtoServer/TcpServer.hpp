#pragma once

#include <iostream>
#include <cstring>
#include <string>
#include <cerrno>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <memory>

#include "Common.hpp"
#include "InetAddr.hpp"
#include "ATMServer.hpp"
using std::cout;
using std::cin;
using std::endl;

#define GBACKLOG 8

//全局端口号
static const uint16_t gport = 2525;
//客户端的地址结构体
InetAddr gclientinet;
class TcpServer
{
public:
    TcpServer(int port = gport) 
    :_port(port), _isrunning(false)
    {

    }
    // 获取一下当前系统的时间
    std::string CurrentTime()
    {
        time_t time_stamp = ::time(nullptr);
        struct tm curr;
        localtime_r(&time_stamp, &curr); // 时间戳，获取可读性较强的时间信息5

        char buffer[1024];
        // bug
        snprintf(buffer, sizeof(buffer), "[%4d-%02d-%02d %02d:%02d:%02d]",
                    curr.tm_year + 1900,
                    curr.tm_mon + 1,
                    curr.tm_mday,
                    curr.tm_hour,
                    curr.tm_min,
                    curr.tm_sec);

        return buffer;
    }
    void InitServer()
    {
        //1.创建tcp_socket ,初始化监听sockt
        //                        ipv4    数据流即tcp
        _listensockfd = ::socket(AF_INET,SOCK_STREAM,0);
        if (_listensockfd < 0)
        {
            cout << "socket error"<<endl;
            Die(SOCKET_ERR);
        }
        //本地socket的定义及初始化
        struct sockaddr_in local;
        memset(&local,0,sizeof(local));
        local.sin_family = AF_INET;
        local.sin_port = htons(gport);
        local.sin_addr.s_addr = INADDR_ANY;

        //2.绑定
        int rn = ::bind(_listensockfd,CONV(&local),sizeof(local));
        if(rn < 0)
        {
            cout << "bind error"<<endl;
            cout<<"bind failed"<< strerror(errno)<<endl;
            Die(BIND_ERR);
        }
        cout << "bind success"<<endl;
        // 3. cs,tcp是面向连接的，就要求tcp随时随地等待被连接
        // tcp 需要将socket设置成为监听状态
        rn = ::listen(_listensockfd, GBACKLOG);
        if(rn < 0)
        {
            cout <<"listen error"<<endl;
        }
        cout << "listen success, sockfd is : " << _listensockfd<<endl;
        //初始化数据库
        _ATMsv = new ATMServer();
        _ATMsv->InitDB();


    }
    //处理请求函数
    void HandleRequest(int sockfd) // TCP也是全双工通信的
    {
        // 初始化ATM有限状态机状态
        _ATMsv->InitStatus();
        cout << "HandlerRequest, sockfd is : " << sockfd<<endl;
        char inbuffer[4096]; 
        // 死循环处理一个用户请求，直到用户主动退出或服务器遇异常关闭
        while(true)
        {
            
            // tcp是连接型，可直接从文件流中读取数据
            ssize_t n = ::read(sockfd, inbuffer, sizeof(inbuffer) - 1);
            // //检查处理前的inbuffer字符
            // for (int i =0 ; i <= n ; ++i) {
            //     std::cout << "inbuffer字符:"<<i<< " ASCII码: " << static_cast<int>(inbuffer[i]) << std::endl;
            // }
            int pos = 0;
            std::string s(inbuffer);
            //处理\r\n
            if((pos = s.find("\r\n", pos)) != std::string::npos)
            {
                n = pos;
            }
            cout<<"s.find pos:"<<pos<< " n:"<< n <<endl;

            if (n > 0)
            {
                // // 添加字符串结束符
                inbuffer[n] = 0;
                // // 检查去掉\n后的字符串
                // for (int i =0 ; i <= n ; ++i)
                // {
                //     std::cout << "处理后inbuffer字符:"<<i<< " ASCII码: " << static_cast<int>(inbuffer[i]) << std::endl;
                // }
                cout<<n<<":" << inbuffer<<endl;
                std::string echo_str;
                // echo_str += inbuffer;
                
                char mscp[basesize];
                strcpy(mscp,inbuffer);
                // 解析报文 以空格为分隔取出每个字符串存入数组
                _ATMsv->ParseMessage(mscp);
                // 打印解析出的报文
                _ATMsv->DeBug();
                // 执行命令,返回值填入echo_str（即向客户端回应的消息）
                echo_str = _ATMsv->ExecuteCommand()._m;
                echo_str += "\n";
                cout<<  (echo_str)<<endl;
                // 将交互记录插入日志数据表
                _ATMsv->InsertLog(CurrentTime(),gclientinet.Addr(),inbuffer,echo_str);

                // 向客户端发送数据
                ::write(sockfd, echo_str.c_str(), echo_str.size());
            }
            else
            {

                // 用户断开连接
                std::cout  << "client quit" << std::endl;
                // 将交互记录插入日志数据表
                _ATMsv->InsertLog(CurrentTime(),gclientinet.Addr(),"NULL","client quit");
                break;
            }
        }
    }
    bool Start()
    {
        _isrunning = true;
        while(true)
        {
            //cout<<1<<endl;
            // 不能直接读取数据
            // 1. 获取新连接
            struct sockaddr_in peer;
            socklen_t peerlen = sizeof(peer);
            cout << "accepting"<<endl;
            // 连接的sockfd（Socket文件描述符，通过TCP传输的数组存储在文件中）
            int sockfd = ::accept(_listensockfd,CONV(&peer),&peerlen);
            if (sockfd < 0)
            {
                cout << "accept error: " << strerror(errno)<<endl;
                return false;
            }
            // 获取连接成功了
            cout << "accept success, sockfd is : " << sockfd<<endl;
            // 获取连接的客户端地址
            InetAddr addr(peer);
            gclientinet = addr;
            cout << CurrentTime()<< "accept success, client info: " << addr.Addr()<<endl;
            std::string echo_s = "accept success: "+addr.Addr();
            // 交互记录插入日志数据表
            _ATMsv->InsertLog(CurrentTime(),gclientinet.Addr(),"NULL",echo_s);
            // 2. 处理请求
            HandleRequest(sockfd);
        }
        //关闭监听套接字
        close(_listensockfd);
        //关闭数据库
        _ATMsv->CloseDB();
        _isrunning =false;
        return true;
    }

private:
    ATMServer* _ATMsv;      // ATMServer对象
    int _listensockfd;      // 监听socketfd
    struct InetAddr _addr;  // 本地socket地址
    uint16_t _port;         // 端口号
    bool _isrunning;        // 服务器是否运行
};
