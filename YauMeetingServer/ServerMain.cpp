#define _CRT_SECURE_NO_WARNINGS
#include<iostream>
#include<cstdlib>


#include"Config.h"
#include"Server.h"

#include<windows.h>
int main(int argc, char* argv[]) {
	const char* userprof = std::getenv("USERPROFILE");
	if (!userprof)exit(0);
	std::string filedirfat = std::string(userprof) + "\\AppData\\Local\\YauMeeting";
	std::string filedir = std::string(userprof)+"\\AppData\\Local\\YauMeeting\\Server\\";

	CreateDirectoryA(filedirfat.c_str(), NULL);
	CreateDirectoryA(filedir.c_str(), NULL);

	Database& db = Database::getInstance();
	db.initDatabase(filedir);

	if (std::string(argv[1]) == "run")
	{
		Server::getInstance().initServer(Config::SERVER_PORT, Config::USER_PORT, Config::MAX_ROOM, filedir);
		Server::getInstance().run();
	}
	else if (std::string(argv[1]) == "register") {
		
		if (argc != 4)
		{
			std::cout << "Usage: register <id> <pwd>" << std::endl;
			return 0;
		}
		std::string id = argv[2];
		std::string pwd = argv[3];

		db.openfile();
		db.adduser(id, pwd);
		db.savefile();
		return 0;
	}
}