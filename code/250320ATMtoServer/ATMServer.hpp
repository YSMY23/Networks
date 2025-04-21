#pragma ONCE
#include "InetAddr.hpp"
#include <mysql/mysql.h>  
#include <string>  
#include <stdio.h>  
#include <string.h> 
#include <fstream>
#include <map> 
using std::cout;
using std::cin;
using std::endl;
using std::cerr;

const char* sep = " ";
const int messagenum = 64;
//全局的报文字段数组
char* gmsv[messagenum];
//全局的报文字段数组元素个数
int gmsc = 0;
const int basesize = 100;

//枚举体，记录用户当前状态
enum 
{
    Ori = 1,
    GetID,
    GetPIN,

};

bool GetCommandLine(char* command_buffer,int size)
{
    //将用户输入的命令行，当作一个完整的字符串
    //ls -a -l
    char * ret = fgets(command_buffer,size,stdin);
    if(!ret)
    {
        return false;
    }
    command_buffer[strlen(command_buffer)-1] =  0;
    if(strlen(command_buffer) == 0)return false;
    return true;
}
struct ResponMessage
{
    const std::string _m; 
    ResponMessage(const char* m)
    :_m(m)
    {
    }
};

class ATMServer
{

public:
    ATMServer()
    {

    }
    //加载配置文件
    std::map<std::string, std::string> loadConfig(const std::string& path) {
        std::map<std::string, std::string> config;
        std::ifstream file(path);
        std::string line, key, value;
        while (std::getline(file, line)) {
            if (line.find('=') != std::string::npos) {
                key = line.substr(0, line.find('='));
                value = line.substr(line.find('=') + 1);
                config[key] = value;
            }
        }
        return config;
    }
    //1.初始化数据库
    int InitDB()
    {
        auto config = loadConfig("../../数据库config/config.ini");
        _my = mysql_init(nullptr);
        if (!_my) {
            cerr << "init MYSQL error" << endl;
            return 1;
        }
        cout << "init MYSQL success" << endl;
        //设置字符编码
        mysql_options(_my, MYSQL_SET_CHARSET_NAME, "utf8");
        mysql_set_character_set(_my, "utf8");
        cout<<config["host"].c_str()<<endl;
        cout<<config["user"].c_str()<<endl;
        cout<<config["password"].c_str()<<endl;
        cout<<config["database"].c_str()<<endl;
        cout<<std::stoi(config["port"])<<endl;

        //连接数据库
        if (// 从配置中获取参数
            mysql_real_connect(_my,
                config["host"].c_str(),
                config["user"].c_str(),
                config["password"].c_str(),
                config["database"].c_str(),
                std::stoi(config["port"]),
                NULL, 0) == NULL)
        {
            cerr << "mysql_real_connect failed" << endl;
            return 1;
        }
        cout << "mysql_real_connect success" << endl;
        return 0;
    }
    //初始化状态
    void InitStatus()
    {
        _status = Ori;
    }
    //2.解析报文，并将每个字段存进报文数组
    void ParseMessage(char* msbuffer)
    {   
        //清除全局数组内容、初始化
        memset(gmsv,0,sizeof(gmsv));
        gmsc = 0;
        //以sep为分隔符取出每一个命令行参数
        //分别取出每个指令字段
        gmsv[gmsc++] =strtok(msbuffer,sep);
        while((bool)(gmsv[gmsc++] = strtok(nullptr,sep)));
        gmsc--;
        return ;

    }
    int InsertLog(std::string time,std::string addr,std::string client_mes,std::string response_mes)
    {
        char inbuffer[300];
        sprintf(inbuffer,"insert into ATMLog values ( '%s', '%s','%s', '%s');"
            ,time.c_str(),addr.c_str(),client_mes.c_str(),response_mes.c_str());
        _query = inbuffer;
        int  rn = mysql_query
        (_my,_query.c_str());
        if (rn != 0)
        {
            std::cerr<<_query << " failed" << endl;
            return 3;
        }
        std::cout << _query << " success" << endl;
        return 0;
    }
    int Query(const char* ID)
    {
        //向数据库发起查询
        _query = "select * from ATMServer where ID=";
        _query+=ID;
        cout<<"Query:"<<_query<<endl;
        int  rn = mysql_query
        (_my,_query.c_str());
        if (rn != 0)
        {
            std::cerr << "MySQL Query Error: " << mysql_error(_my) << std::endl;
            return 3;
        }
        std::cout << _query << " success" << endl;
        //MYSQL_RES *mysql_store_result(MYSQL *mysql);
        //获取结果行数mysql_num_rows
        //my_ulonglong mysql_num_rows(MYSQL_RES * res);
        //获取结果列数mysql_num_fields
        //unsigned int mysql_num_fields(MYSQL_RES * res)
        _res = mysql_store_result(_my);
        //结果集有效性检查
        if (_res == nullptr) {
            // 使用 mysql_error 获取结果集获取失败的原因（如非SELECT语句、内部错误）
            std::cerr << "mysql_store_result failed: " << mysql_error(_my) << std::endl;
            return 1; // 自定义错误码
        }
        _dbrow = mysql_num_rows(_res);
        if (_dbrow == 0) {
            std::cerr << "No _dbrows in result set" << std::endl;
            mysql_free_result(_res); // 释放无效结果集
            return 2;
        }
        _dbfield = mysql_num_fields(_res);
        if (_dbfield == 0) {
            std::cerr << "No fields in result set" << std::endl;
            mysql_free_result(_res); // 释放无效结果集
            return 3;
        }
        _fields = mysql_fetch_fields(_res); // 此时 _fields 应指向有效字段数组

        // for (int i = 0; i < _dbfield; ++i)
        // {
        //     cout << _fields[i].name << "\t";
        // }
        // cout << endl;
        
        // for (int i = 0; i < _dbrow; ++i)
        // {
        //     _rowres = mysql_fetch_row(_res);
        //     for (int j = 0; j < _dbfield; ++j)
        //     {
        //         cout << _rowres[j] << '\t';
        //     }
        //     cout << endl;
        // }
        return 0;
    }
    //3.执行命令            输入的报文数组
    struct ResponMessage ExecuteCommand()
    {
        std::cout<<"start ExecuteCommand _status =="<< _status << endl;
        if(_status == Ori && (strcmp(gmsv[0],"HELO") == 0))
        {
            if(gmsc <2)
            {
                std::cout<<"HELO to which ID?" << endl;
                return "HELO to which ID?";
            }
            ID = gmsv[1];
            cout <<"ID:"<< ID<<endl;
            if(Query(ID.c_str()))
            {
                return "404 NotFound";
            }

            
            if(_dbrow != 0)
            {
                cout <<"HELO success"<<endl;
                _status = GetID;
                return "500 AUTH REQUIRE";
            }
            else
            {
                _status = Ori;
                return "404 NotFound";
            }
        }
        else if(_status == GetID &&(strcmp(gmsv[0],"PASS") == 0))
        {
            if(gmsc <2)
            {
                std::cerr<<"Please input PIN." << endl;
                return "401 ERROR!";
            }
            char* PIN = gmsv[1];
            Query(ID.c_str());
            //查询结果的所有行
            _rowres = mysql_fetch_row(_res);
            if(strcmp(PIN,_rowres[1]) == 0)
            {
                cout << "PIN correct"<<endl;
                _status = GetPIN;
                return ResponMessage("525 OK!");
            }
            else 
            {
                return "401 ERROR!";
            }
            

        }
        else if(_status == GetPIN && (strcmp(gmsv[0],"BALA") == 0))
        {
            Query(ID.c_str());
            //查询结果的所有行
            _rowres = mysql_fetch_row(_res);
            std::string balance = _rowres[2];
            return std::string("AMNT:"+balance).c_str();
        }
        else if(_status = GetPIN && (strcmp(gmsv[0],"WDRA") == 0))
        {
            Query(ID.c_str());
            //查询结果的所有行
            _rowres = mysql_fetch_row(_res);
            double balance = std::stod(_rowres[2]);
            double wdra = std::stod(gmsv[1]);
            if(balance >= wdra)
            {
                char query[basesize];
                sprintf(query,"update ATMServer set balance = %lf where ID=%s",balance-wdra,ID.c_str());
                //向数据库发起查询
                _query = query;
                int  rn = mysql_query
                (_my,_query.c_str());
                if (rn != 0)
                {
                    return "402 database query error";
                }

                return "525 OK";
            }
            else
            {
                return "401 insufficient balance";
            }
        }
        else if((strcmp(gmsv[0],"BYE") == 0))
        {
            _status = Ori;
            return "BYE";
        }
        return "";
    }


    void DeBug()
    {
        cout <<"gmsc=" <<gmsc<<endl;
        
        for(int i = 0 ; i < gmsc ; ++i)
        {
            cout << gmsv[i]<<' ';
        }
        cout <<endl;
    }

    void CloseDB()
    {
        //释放结果集
        if (_res) 
        {
            mysql_free_result(_res);
            _res = nullptr;
        }
        //关闭数据库
        //释放mysql句柄
        if (_my)
        { 
            mysql_close(_my);
            _my = nullptr;
        }
    }

    ~ATMServer()
    {

    }

private:
    MYSQL* _my;//mysql句柄
    std::string _query;//查询语句
    MYSQL_RES* _res;//查询结果
    int _dbrow;//结果的行数
    int _dbfield;//结果的列数
    MYSQL_FIELD* _fields;//结果的列名
    MYSQL_ROW _rowres;    //查询结果的所有行
    std::string ID;
    int _status;
};