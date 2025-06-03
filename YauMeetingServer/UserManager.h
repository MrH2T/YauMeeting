#pragma once
#include<map>
#include<string>
#include"asio.hpp"
class UserManager
{
public:
	std::map<std::string,asio::ip::udp::endpoint> userOnline;
	std::mutex userOnline_mutex;
	UserManager();
	~UserManager();
	void addUserOnline(const std::string& username, const asio::ip::udp::endpoint& endpoint);
	void removeUserOnline(const std::string& username);
	asio::ip::udp::endpoint getUserEndpoint(const std::string& username);
	bool isUserOnline(const std::string& username);
	void clearAllUsers();

};