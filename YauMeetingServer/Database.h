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
	std::map<std::string, std::string> usermap;
	Database(std::string filedir) : filedir(filedir) {}
	~Database();
	void openfile();
	void savefile();
	void adduser(std::string id, std::string pwd);
	bool getUser(std::string id, std::string& pwd);
	bool checkUser(std::string id, std::string pwd);
};

