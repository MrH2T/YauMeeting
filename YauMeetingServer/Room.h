#pragma once
#include<string>
#include<unordered_map>
#include<set>
#include<mutex>
#include<thread>
#include<ctime>
#include<queue>
#include<array>
#include<atomic>

#include"asio.hpp"
class Room
{
public:
	std::unordered_map<std::string, asio::ip::udp::endpoint> users;
	int roomid;
	std::string pwd;
	std::string presenter;

	std::atomic<bool> running;
	std::thread thread_recv, thread_send;

	std::string dialog;
	std::mutex room_mutex;
	int port;
	asio::io_context io_context;
	asio::ip::udp::socket* socket;

	std::queue < std::pair < std::array<char, 1024>, asio::ip::udp::endpoint >> message_queue;
	
	Room(int roomid,int port);
	~Room();

	void spreadMessage(std::array<char, 1024> data, const asio::ip::udp::endpoint& from);
	void handleMessage(std::array<char, 1024> data, const asio::ip::udp::endpoint& from);

	void addUser(const std::string& username, const asio::ip::udp::endpoint & endpoint);
	void removeUser(const std::string& username);
	bool isUserInRoom(const std::string& username);
	std::set<std::string> getUsers();
	void setPresenter(const std::string& presenter);
	void setRoomPassword(const std::string& pwd);
	void setDialog(const std::string& dialog);

	void updateUserList();
	std::string getDialog();
	
	void analysisData();
};

