.PHONY:all
all:server_tcp #client_tcp

server_tcp:TcpServer.cpp
	g++ -o $@ $^ -std=c++17 -lmysqlclient
#client_tcp:TcpClient.cpp
#	g++ -o $@ $^ -std=c++17 -lmysqlclient

ATMServerDebug:ATMServer.cpp
	g++ -o $@ $^ -std=c++17 -lmysqlclient

.PHONY:clean
clean:
	rm -f server_tcp client_tcp ATMServerDebug