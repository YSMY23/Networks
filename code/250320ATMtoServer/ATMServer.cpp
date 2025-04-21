#include<memory>
#include"ATMServer.hpp"

void debugATMServer()
{
    std::unique_ptr<ATMServer> ATMsv = std::make_unique<ATMServer>();
    const int basesize = 1024;
    char msbuffer[basesize];
    ATMsv->InitDB();
    while(GetCommandLine(msbuffer,basesize))
    {
        char mscp[basesize];
        strcpy(mscp,msbuffer);
        ATMsv->ParseMessage(mscp);
        ATMsv->DeBug();

        cout<<ATMsv->ExecuteCommand()._m<<endl;
        

    }



    ATMsv->CloseDB();


}

int main()
{
    
    debugATMServer();
    return 0;
}