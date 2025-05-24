#include "Room.h"
#include"Header.h"

Room::Room(int roomid, int port)
{
	this->roomid = roomid;
	this->socket = new asio::ip::udp::socket(io_context, 
		asio::ip::udp::endpoint(asio::ip::udp::v4(),port));
	this->port = port;
	thread_recv = std::thread([this]() {
		while (true) {
			asio::ip::udp::endpoint sender_endpoint;
			std::array<char, 1024> data;
			size_t length = socket->receive_from(asio::buffer(data), sender_endpoint);
			{
				std::lock_guard<std::mutex> lock(room_mutex);
				message_queue.push({ data, sender_endpoint });
			}

		}
		});
	thread_send = std::thread([this]() {
		while (true) {
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
}

Room::~Room()
{
	if (socket != nullptr) {
		socket->close();
		delete socket;
	}
	io_context.stop();
}

void Room::spreadMessage(std::array<char, 1024> data, const asio::ip::udp::endpoint& from)
{
	std::lock_guard<std::mutex> lock(room_mutex);
	for (const auto& user : users) {
		if (user.second != from) {
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
	TypeHeader* typeheader = new TypeHeader();
	memcpy(typeheader, data.data(), sizeof(TypeHeader));

	if (typeheader->type == 3) {
		DataHeader* dataheader= new DataHeader();
		memcpy(dataheader, data.data() + sizeof(TypeHeader), sizeof(dataheader));


	}
	else if(typeheader->type == 4){
		ControlHeader* controlheader = new ControlHeader();
		memcpy(controlheader, data.data() + sizeof(TypeHeader), sizeof(ControlHeader));

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
}

void Room::removeUser(const std::string& username)
{
	std::lock_guard<std::mutex> lock(room_mutex);
	auto it = users.find(username);
	if (it != users.end()) {
		users.erase(it);
	}
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
