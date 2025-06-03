#pragma once
#include<mutex>
#include<thread>
#include<map>
#include<set>
#include"asio.hpp"
#include"Room.h"

#include"Config.h"
class RoomManager
{
private:
	//from 58000+i
	bool port_available[Config::MAX_ROOM];
	int getAvailablePort();
public:
	std::map<int, Room* > rooms;
	std::mutex room_mutex;
	RoomManager();
	~RoomManager();
	//ret val=port
	int createRoom(int roomid, const std::string& pwd);
	void deleteRoom(int roomid);
	void addUserToRoom(int roomid, const std::string& username,const asio::ip::udp::endpoint& endpoint);
	void removeUserFromRoom(int roomid, const std::string& username);
	void removeUser(const std::string& username);

	bool isUserInRoom(int roomid, const std::string& username);
	bool isRoomExist(int roomid);
	int getRoomPort(int roomid);
	std::set<std::string> getUsersInRoom(int roomid);
	std::string getRoomPassword(int roomid);

	void clearAllRooms();
};

