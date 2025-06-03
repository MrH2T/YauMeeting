#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <cstdlib>
#include <string>

#include"Client.h"

int main(int argc, char* argv[]) {
	if (argc < 2) {
		std::cout << "Usage: YauMeetingClient connect/create" << std::endl;
		return 1;
	}
	if (std::string(argv[1]) == "connect") {
		if (argc != 6 && argc != 7) {
			std::cout << "Usage: YauMeetingClient connect <hostip> <username> <password> <roomid> [<roomdpwd>]" << std::endl;
			return 1;
		}
		std::string hostip(argv[2]);
		std::string username(argv[3]);
		std::string password(argv[4]);
		std::string roomid(argv[5]);
		std::string roompwd = (argc == 7) ? std::string(argv[6]) : std::string();
		int res=Client::getInstance().initClient(hostip,username,password,roomid,roompwd);
		if (res == 0) {
			Client::getInstance().startClient();
		}
		else if(res==1){
			std::cout << "Connection Error." << std::endl;
			return 1;
		}
		else if (res == 2) {
			std::cout << "Cannot connect to Room. Check your roomid and roompwd" << std::endl;
			return 1;
		}
		else if (res == 3) {
			std::cout << "Login error." << std::endl;
			return 1;
		}
	}
	else if (std::string(argv[1]) == "create") {
		if (argc != 5 && argc != 6) {
			std::cout << "Usage: YauMeetingClient create <hostip> <username> <password> [<roomdpwd>]" << std::endl;
			return 1;
		}
		std::string hostip(argv[2]);
		std::string username(argv[3]);
		std::string password(argv[4]);
		std::string roompwd = (argc == 6) ? std::string(argv[5]) : std::string();
		int res = Client::getInstance().initClient(hostip, username, password, "CREATE", roompwd);
		if (res == 0) {
			Client::getInstance().startClient();
		}
		else if (res == 1) {
			std::cout << "Connection Error." << std::endl;
			return 1;
		}
		else if (res == 2) {
			std::cout << "Cannot create room." << std::endl;
			return 1;
		}
		else if (res == 3) {
			std::cout << "Login error." << std::endl;
			return 1;
		}
	}
}