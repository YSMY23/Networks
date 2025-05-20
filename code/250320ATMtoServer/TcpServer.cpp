#include "TcpServer.hpp"


int main()
{
    //智能指针
    std::unique_ptr<TcpServer> tsv = std::make_unique<TcpServer>();

    //初始化服务器
    tsv->InitServer();
    //服务器开始运行
    tsv->Start();


    return 0;
}