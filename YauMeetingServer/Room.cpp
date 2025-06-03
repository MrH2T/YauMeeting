#include "Room.h"
#include"Header.h"
#include"Server.h"

Room::Room(int roomid, int port)
{
	this->roomid = roomid;
	this->socket = new asio::ip::udp::socket(io_context);
	this->socket->open(asio::ip::udp::v4());
	
	asio::ip::udp::endpoint roomendpoint(asio::ip::udp::v4(), port);
	socket->non_blocking(true);
	this->socket->bind(roomendpoint);
	this->port = port;
	this->running = true;
	thread_recv = std::thread([this]() {
		while (this->running) {
			asio::error_code ec;
			asio::ip::udp::endpoint sender_endpoint;
			std::array<char, 1024> data;
			size_t length = socket->receive_from(asio::buffer(data), sender_endpoint,0,ec);
			if (ec == asio::error::would_block) {
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}
			if(!ec){
				std::lock_guard<std::mutex> lock(room_mutex);
				message_queue.push({ data, sender_endpoint });
			}

		}
		});

	thread_send = std::thread([this]() {
		while (this->running) {
			std::pair<std::array<char, 1024>, asio::ip::udp::endpoint> message;
			{
				std::lock_guard<std::mutex> lock(room_mutex);
				if (!message_queue.empty()) {
					message = message_queue.front();
					message_queue.pop();
				}
			}
			if (message_queue.empty()) {
				continue;
			}
			handleMessage(message.first, message.second);
		}
		});
	std::cout << "Yes Room " + std::to_string(roomid) << std::endl;
}

Room::~Room()
{
	running = false;
	if (thread_recv.joinable())thread_recv.join();
	if (thread_send.joinable())thread_send.join();

	if (socket != nullptr) {
		socket->close();
		delete socket;
	}
	io_context.stop();
}

void Room::spreadMessage(std::array<char, 1024> data, const asio::ip::udp::endpoint& from)
{
	//std::cout << "Transfer message" << std::endl;
	std::lock_guard<std::mutex> lock(room_mutex);
	for (const auto& user : users) {
		if (user.second != from) {
			//std::cout << "Send to " + user.first << std::endl;
			socket->send_to(asio::buffer(data), user.second);
		}
	}
}
void Room::handleMessage(std::array<char, 1024> data, const asio::ip::udp::endpoint& from) {
	//this function read the messages from the room users
	//for type 1,2,5, this should not be here and return directly
	//for type 3, spread it at once (but if the mes is a text, which should be read, it's also saved in dialog.)
	//for type 4, this's a control one. We have kick, mute(unmute), stop (for now). 
	//the control message should be specific command with proper format.
	//kick: "kick <username>"
	//mute: "mute <username>"
	//unmute: "unmute <username>"
	//stop: "stop"
	TypeHeader typeheader;
	memcpy(&typeheader, data.data(), sizeof(TypeHeader));

	if (typeheader.type == 3) {
		DataHeader dataheader;
		memcpy(&dataheader, data.data() + sizeof(TypeHeader), sizeof(dataheader));
		// there should have been text, but since DDL is approching, we just forget it :(
		if (dataheader.datatype == 2) {
			//std::cout << "Yes theres audio" << std::endl;
			spreadMessage(data, asio::ip::udp::endpoint());
			//spreadMessage(data, from);
		}
		else {
			spreadMessage(data, asio::ip::udp::endpoint());
		}

	}
	else if(typeheader.type == 4){
		ControlHeader controlheader ;
		memcpy(&controlheader, data.data() + sizeof(TypeHeader), sizeof(ControlHeader));

		// there should have been control command, but since DDL is approching, we just forget it :(
		// we just consider it as the STOP command for now
		char* cmd = new char[controlheader.data_len];
		memcpy(cmd, data.data() + sizeof(TypeHeader) + sizeof(ControlHeader), controlheader.data_len);
		if (cmd[0] == 'S') {
			Server::getInstance().deleteRoom(roomid);
		}
		else if (cmd[0] == 'J') {

		}
		delete[]cmd;
	}

}

void Room::addUser(const std::string& username, const asio::ip::udp::endpoint& endpoint)
{
	std::lock_guard<std::mutex> lock(room_mutex);
	if (users.find(username) == users.end()) {
		users.insert(std::make_pair(username, endpoint));
	}
	else {
		//std::cout << "User " << username << " already in room " << roomid << std::endl;
	}
	updateUserList();
}

void Room::removeUser(const std::string& username)
{
	std::lock_guard<std::mutex> lock(room_mutex);
	auto it = users.find(username);
	if (it != users.end()) {
		users.erase(it);
	}
	else return;


	updateUserList();



	//for(const auto& user : users) {
		
		/*TypeHeader typeheader;
		typeheader.type = 4;
		ControlHeader controlheader;
		std::string cmd = "U" + username;

		controlheader.data_len = cmd.length();

		std::array<char, 1024> data;
		memcpy(data.data(), &typeheader, sizeof(TypeHeader));
		memcpy(data.data() + sizeof(TypeHeader), &controlheader, sizeof(ControlHeader));
		memcpy(data.data() + sizeof(TypeHeader) + sizeof(ControlHeader), cmd.c_str(), cmd.length());
		size_t data_length = sizeof(TypeHeader) + sizeof(ControlHeader) + cmd.length();
		socket->send_to(asio::buffer(data,data_length), user.second);*/
	//}
}

bool Room::isUserInRoom(const std::string& username)
{
	std::lock_guard<std::mutex> lock(room_mutex);
	if (users.find(username) != users.end()) {
		return true;
	}
	return false;
}

std::set<std::string> Room::getUsers()
{
	std::lock_guard<std::mutex> lock(room_mutex);
	if (!users.empty()) {
		std::set<std::string> user_set;
		for (const auto& user : users) {
			user_set.insert(user.first);
		}
		return user_set;
	}
	return std::set<std::string>();
}

void Room::setPresenter(const std::string& presenter)
{
	std::lock_guard<std::mutex> lock(room_mutex);
	this->presenter = presenter;
	//std::cout << "Presenter set to: " << presenter << std::endl;
	//logMessage("Presenter set to: " + presenter);
}

void Room::setRoomPassword(const std::string& pwd)
{
	std::lock_guard<std::mutex> lock(room_mutex);
	this->pwd = pwd;

}

void Room::setDialog(const std::string& dialog)
{
	std::lock_guard<std::mutex> lock(room_mutex);
	this->dialog = dialog;
}

void Room::updateUserList()
{
	TypeHeader typeheader;
	typeheader.type = 4;
	ControlHeader controlheader;
	std::string cmd = "L";
	for (const auto& user : users) {
		cmd += user.first + ";";
	}
	controlheader.data_len = cmd.length();
	std::array<char, 1024> data;
	memcpy(data.data(), &typeheader, sizeof(TypeHeader));
	memcpy(data.data() + sizeof(TypeHeader), &controlheader, sizeof(ControlHeader));
	memcpy(data.data() + sizeof(TypeHeader) + sizeof(ControlHeader), cmd.data(), cmd.size());
	size_t len = sizeof(TypeHeader) + sizeof(ControlHeader) + cmd.size();
	for (const auto& user : users) {
		socket->send_to(asio::buffer(data, len), user.second);
	}
		
}

std::string Room::getDialog()
{
	std::lock_guard<std::mutex> lock(room_mutex);
	if (!dialog.empty()) {
		return dialog;
	}
	return std::string();
}

void Room::analysisData()
{
}
