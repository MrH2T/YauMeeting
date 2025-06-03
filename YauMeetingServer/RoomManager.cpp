#include "RoomManager.h"
#include "Room.h"
#include"Server.h"

int RoomManager::getAvailablePort()
{

	for (int i = 0; i < Config::MAX_ROOM; ++i) {
		if (!port_available[i]) {
			return Config::ROOM_PORT + i;
		}
	}
	return 0;
}

RoomManager::RoomManager() {
}
RoomManager::~RoomManager() {
	std::lock_guard<std::mutex> lock(room_mutex);
	for (auto& room : rooms) {
		delete room.second;
	}
}
int RoomManager::createRoom(int roomid, const std::string& pwd) {
	std::lock_guard<std::mutex> lock(room_mutex);
	if (rooms.find(roomid) == rooms.end()) {
		int port = getAvailablePort();
		if (port == 0) {
			return 0;
		}
		rooms[roomid] = new Room(roomid, port);
		rooms[roomid]->setRoomPassword(pwd);
		port_available[port - Config::ROOM_PORT] = true;
		return port;
	}
	else {
		return rooms[roomid]->port;
	}

}
void RoomManager::deleteRoom(int roomid) {
	std::lock_guard<std::mutex> lock(room_mutex);
	auto it = rooms.find(roomid);
	if (it != rooms.end()) {
		delete it->second;
		rooms.erase(it);
		port_available[it->second->port - Config::ROOM_PORT] = false;
	}
}
void RoomManager::addUserToRoom(int roomid, const std::string& username, const asio::ip::udp::endpoint& endpoint) {
	std::lock_guard<std::mutex> lock(room_mutex);
	auto it = rooms.find(roomid);
	if (it != rooms.end()) {
		it->second->addUser(username,endpoint);
	}
}

void RoomManager::removeUserFromRoom(int roomid, const std::string& username)
{
	std::lock_guard<std::mutex> lock(room_mutex);
	auto it = rooms.find(roomid);
	if (it != rooms.end()) {
		it->second->removeUser(username);
	}
}

bool RoomManager::isUserInRoom(int roomid, const std::string& username)
{
	std::lock_guard<std::mutex> lock(room_mutex);
	auto it = rooms.find(roomid);

	if (it != rooms.end()) {
		return it->second->isUserInRoom(username);
	}
	return false;
}

bool RoomManager::isRoomExist(int roomid)
{
	std::lock_guard<std::mutex> lock(room_mutex);
	auto it = rooms.find(roomid);
	if (it != rooms.end()) {
		return true;
	}
	return false;
}

int RoomManager::getRoomPort(int roomid)
{
	std::lock_guard<std::mutex> lock(room_mutex);
	auto it = rooms.find(roomid);
	if (it != rooms.end()) {
		return it->second->port;
	}
	return 0;
}

std::set<std::string> RoomManager::getUsersInRoom(int roomid)
{
	std::lock_guard<std::mutex> lock(room_mutex);
	auto it = rooms.find(roomid);
	if (it != rooms.end()) {
		return it->second->getUsers();
	}
	return std::set<std::string>();
}
std::string RoomManager::getRoomPassword(int roomid)
{
	std::lock_guard<std::mutex> lock(room_mutex);
	auto it = rooms.find(roomid);
	if (it != rooms.end()) {
		return it->second->pwd;
	}
	return "";
}

void RoomManager::clearAllRooms()
{
	std::lock_guard<std::mutex> lock(room_mutex);
	for (auto& room : rooms) {
		delete room.second;
	}
	rooms.clear();
	for (int i = 0; i < Config::MAX_ROOM; ++i) {
		port_available[i] = false;
	}
}

void RoomManager::removeUser(const std::string& username)
{
	std::lock_guard<std::mutex> lock(room_mutex);
	int pres_roomid = -1;
	for (auto& room : rooms) {
		if( room.second->isUserInRoom(username)) {
			if (room.second->presenter == username) {
				pres_roomid = room.first;
				break;
			}
			else room.second->removeUser(username);
		}
	}
	if(pres_roomid!=-1)Server::getInstance().deleteRoom(pres_roomid);
}