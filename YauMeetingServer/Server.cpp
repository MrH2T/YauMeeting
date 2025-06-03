#include "Server.h"

void Server::initServer(int server_port, int user_port, int max_room, std::string filedir)
{
	io_context.run();


	socket = new asio::ip::udp::socket(io_context);
	socket->open(asio::ip::udp::v4());
	asio::ip::udp::endpoint local(asio::ip::udp::v4(), Config::SERVER_PORT);
	socket->non_blocking(true);
	try {
	socket->bind(local);
	}
	catch (std::exception& e) { std::cout << e.what()<<std::endl; }
	this->server_port = server_port;
	this->user_port = user_port;
	this->max_room = max_room;
	this->filedir = filedir;

	log = new Logger();
	roomManager = new RoomManager();
	userManager = new UserManager();
	Database::getInstance().openfile();

	

	
}
Server::Server()
{

}
Server::~Server() {
	running = false;
	log->log("Lets Wait");
	if (thread_recv.joinable())thread_recv.join();
	log->log("Yeah");

	auto users = userManager->userOnline;
	for (const auto& user:users) {
		// Notify each user in the room that the room is being deleted
		asio::ip::udp::endpoint endpoint = user.second;
		if (endpoint.address().is_v4()) {
			TypeHeader typeheader;
			typeheader.type = 3; // Room deletion notification type
			ControlHeader controlheader;
			std::string cmd = "STOP";
			controlheader.data_len = cmd.length();

			std::array<char, 1024> response_data;
			memcpy(response_data.data(), &typeheader, sizeof(TypeHeader));
			memcpy(response_data.data() + sizeof(TypeHeader), &controlheader, sizeof(ControlHeader));
			memcpy(response_data.data() + sizeof(TypeHeader) + sizeof(ControlHeader), cmd.data(), cmd.size());
			size_t len = sizeof(TypeHeader) + sizeof(ControlHeader) + cmd.size();
			socket->send_to(asio::buffer(response_data, len), endpoint);
		}
	}
	userManager->clearAllUsers();
	roomManager->clearAllRooms();



	io_context.stop();



	delete socket;
	delete log;
	delete roomManager;
	delete userManager;
}


void Server::run() {
	running = true;
	thread_recv = std::thread([this]() {
		while (this->running) {
			asio::error_code ec;
			asio::ip::udp::endpoint sender_endpoint;
			std::array<char, 1024> data;
			size_t length = socket->receive_from(asio::buffer(data), sender_endpoint,0,ec);
			if (ec == asio::error::would_block) {
				std::this_thread::sleep_for(std::chrono::milliseconds(5));
				continue;
			}
			if (!ec) {
				handleMessage(data,sender_endpoint);
			}
		}
		});

	logMessage("Server Started Running...");

	while (1) {
		std::string command;
		std::cin >> command;
		if (command == "STOP")exit(0);
	}
}

void Server::createRoom(int roomid, const std::string& pwd)
{
	roomManager->createRoom(roomid, pwd);
	logMessage("Room " + std::to_string(roomid) + " created with password: " + pwd);
}

void Server::deleteRoom(int roomid)
{
	auto users = roomManager->getUsersInRoom(roomid);
	for (const auto& user : users) {
		// Notify each user in the room that the room is being deleted
		asio::ip::udp::endpoint endpoint = userManager->getUserEndpoint(user);
		if (endpoint.address().is_v4()) {
			TypeHeader typeheader;
			typeheader.type = 3; // Room deletion notification type
			ControlHeader controlheader;
			std::string cmd = "STOP";
			controlheader.data_len = cmd.length();

			std::array<char, 1024> response_data;
			memcpy(response_data.data(), &typeheader, sizeof(TypeHeader));
			memcpy(response_data.data() + sizeof(TypeHeader), &controlheader, sizeof(ControlHeader));
			memcpy(response_data.data() + sizeof(TypeHeader) + sizeof(ControlHeader), cmd.data(), cmd.size());
			size_t len = sizeof(TypeHeader) + sizeof(ControlHeader) + cmd.size();
			socket->send_to(asio::buffer(response_data, len), endpoint);
		}
	}
	
	roomManager->deleteRoom(roomid);

	logMessage("Room " + std::to_string(roomid) + " deleted");
}

void Server::addUserToRoom(int roomid, const std::string& username, const asio::ip::udp::endpoint &endpoint)
{
	roomManager->addUserToRoom(roomid, username, endpoint);
	logMessage("User " + username + " added to room " + std::to_string(roomid));
}

void Server::removeUserFromRoom(int roomid, const std::string& username)
{
	roomManager->removeUserFromRoom(roomid, username);
	logMessage("User " + username + " removed from room " + std::to_string(roomid));
}

bool Server::isUserInRoom(int roomid, const std::string& username)
{
	std::lock_guard<std::mutex> lock(roomManager->room_mutex);
	if (roomManager->rooms.find(roomid) != roomManager->rooms.end())
	{
		return roomManager->isUserInRoom(roomid, username);
	}
	return false;
}

std::set<std::string> Server::getUsersInRoom(int roomid)
{
	std::lock_guard<std::mutex> lock(roomManager->room_mutex);
	if (roomManager->rooms.find(roomid) != roomManager->rooms.end())
	{
		return roomManager->getUsersInRoom(roomid);
	}
	return std::set<std::string>();

}

std::string Server::getRoomPassword(int roomid)
{
	std::lock_guard<std::mutex> lock(roomManager->room_mutex);
	if (roomManager->rooms.find(roomid) != roomManager->rooms.end())
	{
		return roomManager->getRoomPassword(roomid);
	}
	return std::string();
}

void Server::addUserOnline(const std::string& username, const asio::ip::udp::endpoint& endpoint)
{
	userManager->addUserOnline(username, endpoint);
	logMessage("User " + username + " is online");
}

void Server::removeUserOnline(const std::string& username)
{
	userManager->removeUserOnline(username);
	logMessage("User " + username + " is offline");
}

asio::ip::udp::endpoint Server::getUserEndpoint(const std::string& username)
{
	std::lock_guard<std::mutex> lock(userManager->userOnline_mutex);
	if (userManager->userOnline.find(username) != userManager->userOnline.end())
	{
		return userManager->getUserEndpoint(username);
	}
	return asio::ip::udp::endpoint();
}

bool Server::isUserOnline(const std::string& username)
{
	std::lock_guard<std::mutex> lock(userManager->userOnline_mutex);
	if (userManager->userOnline.find(username) != userManager->userOnline.end())
	{
		return userManager->isUserOnline(username);
	}
	return false;
}

void Server::logMessage(const std::string& message)
{
	log->log(message);
	
}

void Server::handleMessage(std::array<char, 1024> msg, const asio::ip::udp::endpoint &sender_endpoint)
{
	TypeHeader typeheader;
	memcpy(&typeheader, msg.data(), sizeof(TypeHeader));
	if (typeheader.type == 1) {
		LoginHeader loginheader;
		memcpy(&loginheader, msg.data() + sizeof(TypeHeader), sizeof(LoginHeader));

		//login header has username_len and password_len. We should read from the following of msg, with two lengths,
		//and then we can get the username and password. Then we should check from database whether they are correct.
		//furthermore, if check is failed, return a LoginResponse with 127;
		//if check ok, we should decide the login type.
		//first is connect to existing room, which we excerpt roomid, and with given len to read the given password.
		//then check the existence and the given pwd's correctness. If room not exist, return 63; if pwd not correct, return 64
		//if ok, return the corresponding port of the room in result.

		std::string username = std::string(msg.data() + sizeof(TypeHeader) + sizeof(LoginHeader), loginheader.username_len);
		std::string password = std::string(msg.data() + sizeof(TypeHeader) + sizeof(LoginHeader) + loginheader.username_len, loginheader.password_len);

		log->log(username + " " + password);
		if (!Database::getInstance().checkUser(username, password)) {
			TypeHeader rettypeheader;
			rettypeheader.type = 2;
			
			//return error code 127
			
			LoginResponseHeader loginresponse;
			loginresponse.result = 127;
			loginresponse.room_id = 0;

			//make a new data buffer with the typeheader and loginresponse header
			std::array<char, 1024> response_data;
			memcpy(response_data.data(), &typeheader, sizeof(TypeHeader));
			memcpy(response_data.data() + sizeof(TypeHeader), &loginresponse, sizeof(LoginResponseHeader));

			size_t len = sizeof(TypeHeader) + sizeof(LoginResponseHeader);

			socket->send_to(asio::buffer(response_data, len), sender_endpoint);
			log->log("I send 127");
			return;
		}
		log->log("Oh god he passed");
		uint8_t login_type = loginheader.type;

		TypeHeader rettypeheader;
		rettypeheader.type = 2;

		uint8_t roomid = loginheader.room_id;
		std::string room_pwd = std::string(msg.data() + sizeof(TypeHeader) + sizeof(LoginHeader) + loginheader.username_len + loginheader.password_len, loginheader.roompwd_len);

		LoginResponseHeader loginresponse;
		loginresponse.room_id = roomid;
		log->log("He want "+std::to_string(login_type));

		if (login_type == 1) {
			bool isok = 1;

			//connect to existing room
			if(roomManager->getRoomPassword(roomid) == room_pwd) {
				//check if the room exists and the password is correct
				if (roomManager->isRoomExist(roomid)) {
					//return the port of the room
					loginresponse.result = roomManager->getRoomPort(roomid);
				}
				else {
					//return error code 63
					loginresponse.result = 63;
					isok = 0;
				}
			}
			else {
				loginresponse.result = 64;
				isok = 0;
			}

			//make a new data buffer with the typeheader and loginresponse header 
			std::array<char, 1024> response_data;
			memcpy(response_data.data(), &rettypeheader, sizeof(TypeHeader));
			memcpy(response_data.data() + sizeof(TypeHeader), &loginresponse, sizeof(LoginResponseHeader));

			size_t len = sizeof(TypeHeader) + sizeof(LoginResponseHeader);
			log->log("I will send ~ ");
			socket->send_to(asio::buffer(response_data, len), sender_endpoint);
			log->log("I send "+std::to_string(loginresponse.result));
			if(isok) {
				//add user to online list
				userManager->addUserOnline(username, sender_endpoint);
				//add user to room
				roomManager->addUserToRoom(roomid, username, sender_endpoint);
				roomManager->rooms[roomid]->setPresenter(username);
			}
		}
		else if (login_type == 2) {
			bool isok = 1;
			//create new room
			std::string room_pwd = std::string(msg.data() + sizeof(TypeHeader) + sizeof(LoginHeader) + loginheader.username_len + loginheader.password_len, loginheader.roompwd_len);
			int res=roomManager->createRoom(roomid, room_pwd);
			if (res == 0) {
				//cannot create room, return error code 63
				loginresponse.result = 63;
				isok = 0;
			}
			else {
				//return the port of the room
				loginresponse.result = roomManager->getRoomPort(roomid);
			}
			//make a new data buffer with the typeheader and loginresponse header
			std::array<char, 1024> response_data;
			memcpy(response_data.data(), &rettypeheader, sizeof(TypeHeader));
			memcpy(response_data.data() + sizeof(TypeHeader), &loginresponse, sizeof(LoginResponseHeader));
			size_t len = sizeof(TypeHeader) + sizeof(LoginResponseHeader);
			log->log("I will send ");
			socket->send_to(asio::buffer(response_data, len), sender_endpoint);
			log->log("I send " + std::to_string(loginresponse.result));
			if(isok) {
				//add user to online list
				userManager->addUserOnline(username, sender_endpoint);
				//add user to room
				roomManager->addUserToRoom(roomid, username, sender_endpoint);
			}
		
		}

	}
	else if (typeheader.type == 5) {
		//LogoffHeader has username_len, we should read the username from the following of msg.
		//then remove the user from the userOnline list, and also remove the user from the room.

		LogoffHeader logoffheader;
		memcpy(&logoffheader, msg.data() + sizeof(TypeHeader), sizeof(LogoffHeader));
		std::string username = std::string(msg.data() + sizeof(TypeHeader) + sizeof(LogoffHeader), logoffheader.username_len);
		userManager->removeUserOnline(username);
		//remove the user from the room
		roomManager->removeUser(username);
	}
}

