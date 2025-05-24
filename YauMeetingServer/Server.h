#pragma once

#include<iostream>
#include<string>
#include<thread>
#include<mutex>
#include<utility>
#include<array>

#include"asio.hpp"

#include"Logger.h"
#include"Database.h"
#include"RoomManager.h"
#include"UserManager.h"
#include"Room.h"
#include"Header.h"



class Server
{
public:
	asio::io_context io_context;



	Server(int server_port, int user_port, int max_room, std::string filedir);
	~Server();


	void run();
	void createRoom(int roomid, const std::string& pwd);
	void deleteRoom(int roomid);
	void addUserToRoom(int roomid, const std::string& username, const asio::ip::udp::endpoint& endpoint);
	void removeUserFromRoom(int roomid, const std::string& username);
	bool isUserInRoom(int roomid, const std::string& username);
	std::set<std::string> getUsersInRoom(int roomid);
	std::string getRoomPassword(int roomid);
	void addUserOnline(const std::string& username, const asio::ip::udp::endpoint& endpoint);
	void removeUserOnline(const std::string& username);
	asio::ip::udp::endpoint getUserEndpoint(const std::string& username);
	bool isUserOnline(const std::string& username);
	void logMessage(const std::string& message);

	

private:
	int server_port, user_port;
	int max_room;
	std::string filedir;
	Database* db;
	Logger* log;
	RoomManager* roomManager;
	UserManager* userManager;

	std::thread thread_recv;
	asio::ip::udp::socket* socket;

	void handleMessage(std::array<char, 1024> msg,const asio::ip::udp::endpoint &sender_endpoint);
	
};

