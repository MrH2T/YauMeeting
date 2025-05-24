#pragma once
#include<string>
#include<iostream>
#include<map>

/*
* A database txt file contains the following:
* ID PWD
* ...
*/



class Database
{
private:
	std::string filedir;
public:
	static Database& getInstance()
	{
		static Database instance;
		return instance;
	}
	Database();
	void initDatabase(std::string filedir);
	~Database();

	std::map<std::string, std::string> usermap;
	void openfile();
	void savefile();
	void adduser(std::string id, std::string pwd);
	bool getUser(std::string id, std::string& pwd);
	bool checkUser(std::string id, std::string pwd);
};

