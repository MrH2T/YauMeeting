#include "UserManager.h"

UserManager::UserManager()
{
}
UserManager::~UserManager()
{
}
void UserManager::addUserOnline(const std::string& username, const asio::ip::udp::endpoint& endpoint)
{
	userOnline[username] = endpoint;
}
void UserManager::removeUserOnline(const std::string& username)
{
	userOnline.erase(username);
}
asio::ip::udp::endpoint UserManager::getUserEndpoint(const std::string& username)
{
	auto it = userOnline.find(username);
	if (it != userOnline.end())
	{
		return it->second;
	}
	else
	{
		throw std::runtime_error("User not found");
	}
}
bool UserManager::isUserOnline(const std::string& username)
{
	return userOnline.find(username) != userOnline.end();
}

void UserManager::clearAllUsers()
{
	userOnline.clear();
}

