#include "TcpServer.hpp"


int main()
{
    //智能指针
    std::unique_ptr<TcpServer> tsv = std::make_unique<TcpServer>();

    tsv->InitServer();
    tsv->Start();


    return 0;
}