#pragma ONCE
#include "InetAddr.hpp"
#include <mysql/mysql.h>
#include <string>
#include <stdio.h>
#include <string.h>
#include <fstream>
#include <map>
using std::cerr;
using std::cin;
using std::cout;
using std::endl;

const char *sep = " ";
const int messagenum = 64;
// 全局的报文字段数组
char *gmsv[messagenum];
// 全局的报文字段数组元素个数
int gmsc = 0;
const int basesize = 100;

// 枚举体，记录用户当前状态
enum
{
    Ori = 1,
    GetID,
    GetPIN,

};

bool GetCommandLine(char *command_buffer, int size)
{
    // 将用户输入的命令行，当作一个完整的字符串
    // ls -a -l
    char *ret = fgets(command_buffer, size, stdin);
    if (!ret)
    {
        return false;
    }
    command_buffer[strlen(command_buffer) - 1] = 0;
    if (strlen(command_buffer) == 0)
        return false;
    return true;
}
struct ResponMessage
{
    const std::string _m;
    ResponMessage(const char *m)
        : _m(m)
    {
    }
};

class ATMServer
{

public:
    ATMServer()
    {
    }
    // 加载数据库配置文件
    std::map<std::string, std::string> loadConfig(const std::string &path)
    {
        std::map<std::string, std::string> config;
        std::ifstream file(path);
        std::string line, key, value;
        while (std::getline(file, line))
        {
            if (line.find('=') != std::string::npos)
            {
                key = line.substr(0, line.find('='));
                value = line.substr(line.find('=') + 1);
                config[key] = value;
            }
        }
        return config;
    }
    // 1.初始化数据库
    int InitDB()
    {
        auto config = loadConfig("../../数据库config/config.ini");
        _my = mysql_init(nullptr);
        if (!_my)
        {
            cerr << "init MYSQL error" << endl;
            return 1;
        }
        cout << "init MYSQL success" << endl;
        // 设置字符编码
        mysql_options(_my, MYSQL_SET_CHARSET_NAME, "utf8");
        mysql_set_character_set(_my, "utf8");
        // cout<<config["host"].c_str()<<endl;
        // cout<<config["user"].c_str()<<endl;
        // cout<<config["password"].c_str()<<endl;
        // cout<<config["database"].c_str()<<endl;
        // cout<<std::stoi(config["port"])<<endl;

        // 连接数据库
        if ( // 从配置中获取参数
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
    // 初始化状态
    void InitStatus()
    {
        _status = Ori;
    }
    // 2.解析报文，并将每个字段存进报文数组
    void ParseMessage(char *msbuffer)
    {
        // 清除全局数组内容、初始化
        memset(gmsv, 0, sizeof(gmsv));
        gmsc = 0;
        // 以sep为分隔符取出每一个命令行参数
        // 分别取出每个指令字段
        gmsv[gmsc++] = strtok(msbuffer, sep);
        while ((bool)(gmsv[gmsc++] = strtok(nullptr, sep)))
            ;
        gmsc--;
        return;
    }
    // 记录日志函数
    int InsertLog(std::string time, std::string addr, std::string client_mes, std::string response_mes)
    {
        char inbuffer[300];
        sprintf(inbuffer, "insert into ATMLog values ( '%s', '%s','%s', '%s');", time.c_str(), addr.c_str(), client_mes.c_str(), response_mes.c_str());
        _query = inbuffer;
        int rn = mysql_query(_my, _query.c_str());
        if (rn != 0)
        {
            std::cerr << _query << " failed" << endl;
            return 3;
        }
        std::cout << _query << " success" << endl;
        return 0;
    }
    // 查询数据库函数
    int Query(const char *ID)
    {
        // 向数据库发起查询
        _query = "select * from ATMServer where ID=";
        _query += ID;
        // cout << "Query:" << _query << endl;
        int rn = mysql_query(_my, _query.c_str());
        if (rn != 0)
        {
            std::cerr << "MySQL Query Error: " << mysql_error(_my) << std::endl;
            return 3;
        }
        // std::cout << _query << " success" << endl;
        // 获取结果集
        _res = mysql_store_result(_my);
        // 结果集有效性检查
        if (_res == nullptr)
        {
            // 使用 mysql_error 获取结果集获取失败的原因（如非SELECT语句、内部错误）
            std::cerr << "mysql_store_result failed: " << mysql_error(_my) << std::endl;
            return 1; // 自定义错误码
        }
        _dbrow = mysql_num_rows(_res);
        if (_dbrow == 0)
        {
            std::cerr << "No _dbrows in result set" << std::endl;
            mysql_free_result(_res); // 释放无效结果集
            return 2;
        }
        _dbfield = mysql_num_fields(_res);
        if (_dbfield == 0)
        {
            std::cerr << "No fields in result set" << std::endl;
            mysql_free_result(_res); // 释放无效结果集
            return 3;
        }
        _fields = mysql_fetch_fields(_res); // 此时 _fields 应指向有效字段数组

        return 0;
    }
    // 3.执行命令           （输入的报文数组为命令）
    struct ResponMessage ExecuteCommand()
    {
        std::cout << "start ExecuteCommand _status ==" << _status << endl;
        // _status == Ori 且客户端发送 HELO <userid>时
        if (_status == Ori)
        {
            if ((strcmp(gmsv[0], "HELO") == 0))
            {
                if (gmsc < 2)
                {
                    std::cout << "HELO to which ID?" << endl;
                    return "HELO to which ID?";
                }
                ID = gmsv[1];
                cout << "ID:" << ID << endl;
                if (Query(ID.c_str()))
                {
                    return "404 NotFound";
                }

                if (_dbrow != 0)
                {
                    cout << "HELO success" << endl;
                    _status = GetID;
                    return "500 AUTH REQUIRE";
                }
                else
                {
                    _status = Ori;
                    return "404 NotFound";
                }
            }
            // 客户端发送 BYE 时
            else if ((strcmp(gmsv[0], "BYE") == 0))
            {
                _status = Ori;
                return "BYE";
            }
            else
            {
                std::cout << "Please input HELO <userid>" << endl;
                return "Please input HELO <userid>";
            }
        }
        // _status == GetID 且客户端发送 PASS <passwd>时
        else if (_status == GetID)
        {
            if ((strcmp(gmsv[0], "PASS") == 0))
            {
                if (gmsc < 2)
                {
                    std::cerr << "Please input PIN." << endl;
                    return "401 ERROR!";
                }
                char *PIN = gmsv[1];
                Query(ID.c_str());
                // 查询结果的所有行
                _rowres = mysql_fetch_row(_res);
                if (strcmp(PIN, _rowres[1]) == 0)
                {
                    cout << "PIN correct" << endl;
                    _status = GetPIN;
                    return ResponMessage("525 OK!");
                }
                else
                {
                    cout << "PIN incorrect!"<<endl;
                    cout<<"C_PIN=" << PIN << "PIN= " << _rowres[1] << endl;
                    int i =0;
                    while(PIN[i])
                    {
                        std::cout << "PIN字符:" << i << " ASCII码: " << static_cast<int>(PIN[i]) << std::endl;
                        ++i;
                    }
                    i = 0;
                    while(_rowres[1][i])
                    {
                        std::cout << "_rowres字符:" << i << " ASCII码: " << static_cast<int>(_rowres[1][i]) << std::endl;
                        ++i;
                    }
                    return "401 ERROR!";
                }
            }
            // 客户端发送 BYE 时
            else if ((strcmp(gmsv[0], "BYE") == 0))
            {
                _status = Ori;
                return "BYE";
            }
            else
            {
                std::cerr << "Please input PASS <passwd>" << endl;
                return "401 ERROR!";
            }
        }
        // _status == GetPIN 且客户端发送 BALA时
        else if (_status == GetPIN)
        {
            if ((strcmp(gmsv[0], "BALA") == 0))
            {
                Query(ID.c_str());
                // 查询结果的所有行
                _rowres = mysql_fetch_row(_res);
                std::string balance = _rowres[2];
                return std::string("AMNT:" + balance).c_str();
            }
            // _status == GetPIN 且客户端发送 WDRA <amnt> 时
            else if ((strcmp(gmsv[0], "WDRA") == 0))
            {
                Query(ID.c_str());
                // 查询结果的所有行
                _rowres = mysql_fetch_row(_res);
                double balance = std::stod(_rowres[2]);
                double wdra = std::stod(gmsv[1]);
                if (balance >= wdra)
                {
                    char query[basesize];
                    sprintf(query, "update ATMServer set balance = %lf where ID=%s", balance - wdra, ID.c_str());
                    // 向数据库发起查询
                    _query = query;
                    int rn = mysql_query(_my, _query.c_str());
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
            // 客户端发送 BYE 时
            else if ((strcmp(gmsv[0], "BYE") == 0))
            {
                _status = Ori;
                return "BYE";
            }
            else
            {
                std::cerr << "Please input BALA or WDRA <amnt>" << endl;
                return "401 ERROR!";
            }
        }
        // 客户端发送其他命令时
        cout << "[In the ExecuteCommand]:_status ==" << _status << "==>";
        cout << gmsv[0];
        for (int i = 1; i < gmsc; ++i)
        {
            cout << " " << gmsv[i];
        }
        cout << endl;
        return "404 NotFound";
    }

    void DeBug()
    {
        cout << "gmsc=" << gmsc << endl;

        for (int i = 0; i < gmsc; ++i)
        {
            cout << gmsv[i] << ' ';
        }
        cout << endl;
    }

    // 清理数据库有关资源
    void CloseDB()
    {
        // 释放结果集
        if (_res)
        {
            mysql_free_result(_res);
            _res = nullptr;
        }
        // 关闭数据库
        // 释放mysql句柄
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
    MYSQL *_my;           // mysql句柄
    std::string _query;   // 查询语句
    MYSQL_RES *_res;      // 查询结果
    int _dbrow;           // 结果的行数
    int _dbfield;         // 结果的列数
    MYSQL_FIELD *_fields; // 结果的列名
    MYSQL_ROW _rowres;    // 查询结果的所有行
    std::string ID;       // 用户ID
    int _status;          // 当前状态
};