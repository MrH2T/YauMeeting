#include "Database.h"
#include<fstream>
Database::~Database()
{
	savefile();
}
void Database::adduser(std::string id, std::string pwd)
{
	if (usermap.find(id) != usermap.end())
	{
		std::cout << "User already exists" << std::endl;
		return;
	}
	usermap[id] = pwd;
	std::cout << "User added" << std::endl;
}
void Database::openfile()
{
	std::ifstream file(filedir + "user.txt");
	if (!file.is_open())
	{
		return;
	}
	std::string id, pwd;
	while (file >> id >> pwd)
	{
		usermap[id] = pwd;
	}
	file.close();
	std::cout << "File opened" << std::endl;
}
void Database::savefile()
{
	std::ofstream file(filedir + "user.txt", std::ios::out|std::ios::trunc);
	if (!file.is_open())
	{
		std::cout << "File not opened" << std::endl;
		return;
	}
	for (auto it = usermap.begin(); it != usermap.end(); it++)
	{
		file << it->first << " " << it->second << std::endl;
	}
	file.close();
	std::cout << "File saved" << std::endl;
}
bool Database::getUser(std::string id, std::string& pwd)
{
	if (usermap.find(id) == usermap.end())
	{
		return false;
	}
	pwd = usermap[id];
	return true;
}

bool Database::checkUser(std::string id, std::string pwd)
{
	std::string real_pwd;
	if (!getUser(id, real_pwd))return false;
	return real_pwd == pwd;
}
