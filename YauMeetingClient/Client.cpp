#include "Client.h"
#include"Header.h"
#include"Config.h"
#include"DataProcessor.h"
#include<array>

int Client::tryConnect()
{
	TypeHeader typeheader;
	typeheader.type = 1;

	LoginHeader loginHeader;
	loginHeader.username_len = username.length();
	loginHeader.password_len = userpwd.length();
	loginHeader.type = (roomid == 0) ? 2 : 1;
	loginHeader.room_id = roomid;
	loginHeader.roompwd_len = roompwd.length();

	std::string msg = username + userpwd + roompwd;
	std::array<char, 1024> data;
	memcpy(data.data(), &typeheader, sizeof(TypeHeader));
	memcpy(data.data() + sizeof(TypeHeader), &loginHeader, sizeof(LoginHeader));
	memcpy(data.data() + sizeof(TypeHeader) + sizeof(LoginHeader), msg.data(), msg.size());

	size_t len = sizeof(TypeHeader) + sizeof(LoginHeader) + msg.size();
	
	asio::ip::udp::endpoint remote(asio::ip::make_address(hostip), Config::SERVER_PORT),
		local(asio::ip::udp::v4(),Config::USER_PORT), localsend(asio::ip::udp::v4(), Config::USER_SEND_PORT);
	socket->open(asio::ip::udp::v4());
	socket_send->open(asio::ip::udp::v4());
	socket->bind(local);
	std::array<char, 1024> recv;
	try {
		socket->send_to(asio::buffer(data, len), remote);
		socket->receive_from(asio::buffer(recv), serverEndpoint);
	}
	catch(...){
		sendLogoff();
		return 1;
	}
	socket->non_blocking(true);

	TypeHeader recvtypeheader;
	memcpy(&recvtypeheader, recv.data(), sizeof(TypeHeader));
	std::cout << recvtypeheader.type << std::endl;
	if (recvtypeheader.type != 2)exit(1);

	LoginResponseHeader response;
	memcpy(&response, recv.data() + sizeof(TypeHeader), sizeof(LoginResponseHeader));
	if (response.result == 127)return 3;
	else if (response.result == 63)return 2;
	else if (response.result == 64)return 2;

	roomEndpoint = asio::ip::udp::endpoint(asio::ip::make_address(hostip), response.result);
	roomid = response.room_id;


	return 0;
}

Client::Client()
{
}

Client::~Client()
{
	Pa_StopStream(istream);
	Pa_StopStream(ostream);

	running = false;
	if (thread_toroom.joinable())thread_toroom.join();
	if (thread_recv.joinable())thread_recv.join();

	cap.release();

	io_context.stop();
	delete socket;
	delete socket_send;
	delete frame;

}

int Client::initClient(std::string hostip, std::string username, std::string userpwd, std::string roomid, std::string roompwd)
{
	this->hostip = hostip;
	this->username = username;
	this->userpwd = userpwd;
	if (roomid != "CREATE")this->roomid = std::atoi(roomid.c_str());
	else this->roomid = 0;
	this->roompwd = roompwd;

	socket = new asio::ip::udp::socket(io_context);
	socket_send = new asio::ip::udp::socket(io_context);

	return tryConnect();
}

void Client::startClient()
{

	running = true;
	frame = new MainFrame(username, roomid);
	
	thread_recv= std::thread([this]() {
		while (running) {
			asio::error_code ec;
			asio::ip::udp::endpoint sender;
			std::array<char, 1024> data;
			size_t length = socket->receive_from(asio::buffer(data), sender, 0, ec);
			if (ec == asio::error::would_block) {
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
				continue;
			}
			if (!ec) {
				handleMessage(data);
			}
		}
		});
	thread_toroom = std::thread([this]() {
		while (running) {
			videoSend();
			audioSend();

			std::this_thread::sleep_for(std::chrono::milliseconds(25));//40FPS
		}
		});

	cap.open(0);
	if (!cap.isOpened()) {
		sendLogoff();
		exit(0);
	}
	Pa_Initialize();
	Pa_OpenDefaultStream(&istream, NUM_CHANNELS, 0, paFloat32, SAMPLE_RATE,
		FRAMES_PER_BUFFER, recordCallback, &DataProcessor::getInstance().audioBuffer);
	Pa_StartStream(istream);

	Pa_OpenDefaultStream(&ostream, 0, 1, paFloat32, SAMPLE_RATE, FRAMES_PER_BUFFER, playCallback, nullptr);
	Pa_StartStream(ostream);

	frame->run();
	/*while (running) {

	}*/
}

void Client::videoSend()
{
	cv::Mat frame;
	cap >> frame;
	//if (frame.empty())std::cout << "Oh fuck it cant work" << std::endl;
	if (frame.empty())return;
	uint64_t timestamp = static_cast<uint64_t>(cv::getTickCount());
	DataProcessor::getInstance().videoProcess(frame, username, timestamp);
}

void Client::audioSend()
{
	uint64_t timestamp = static_cast<uint64_t>(cv::getTickCount());
	DataProcessor::getInstance().audioProcess(DataProcessor::getInstance().audioBuffer, username, timestamp);
}

void Client::handleMessage(std::array<char, 1024> data)
{
	TypeHeader typeheader;
	memcpy(&typeheader, data.data(), sizeof(TypeHeader));
	if (typeheader.type == 3) {
		DataProcessor::getInstance().handlePacket(data);

	}
	else if (typeheader.type == 4) {
		ControlHeader controlheader;
		memcpy(&controlheader, data.data() + sizeof(TypeHeader), sizeof(ControlHeader));
		char* cmd = new char[controlheader.data_len];
		memcpy(cmd, data.data() + sizeof(TypeHeader) + sizeof(ControlHeader), controlheader.data_len);
		if (cmd[0] == 'S') {
			sendLogoff();
			stop();
			exit(0);
		}
		else if (cmd[0] == 'L') {
			std::string userlist(cmd + 1);
			updateUserList(userlist);
			std::cout << "Yes I received " + userlist << std::endl;
		}
		delete[]cmd;
	}
}


Client& Client::getInstance() {
	static Client instance;
	return instance;
}

void Client::updateUserList(std::string list) {
	std::string name = "";
	std::vector<std::string>usernames;
	for (int i = 0; i < list.length(); i++) {
		if (list[i] == ';')users.insert(name), usernames.push_back(name), name = "";
		else name += list[i];
	}
	//TODO frame update 
	
	frame->updateUserList(usernames);
}
void Client::sendLogoff() {
	TypeHeader typeheader;
	typeheader.type = 5;
	LogoffHeader logoffheader;
	logoffheader.username_len = username.length();
	std::string cmd = username;
	std::array<char, 1024> data;
	memcpy(data.data(), &typeheader, sizeof(TypeHeader));
	memcpy(data.data() + sizeof(TypeHeader), &logoffheader, sizeof(LogoffHeader));
	memcpy(data.data() + sizeof(TypeHeader) + sizeof(LogoffHeader), cmd.data(), cmd.size());
	size_t len = sizeof(TypeHeader) + sizeof(LogoffHeader) + cmd.size();
	socket->send_to(asio::buffer(data, len), serverEndpoint);
}

int Client::recordCallback(const void* inputBuffer, void*, unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo*, PaStreamCallbackFlags, void* userData)
{
	std::lock_guard<std::mutex> locker(DataProcessor::getInstance().datamutex);
	auto* buffer = static_cast<std::vector<SAMPLE>*>(userData);
	const SAMPLE* in = static_cast<const SAMPLE*> (inputBuffer);
	buffer->insert(buffer->end(), in, in + framesPerBuffer);
	return paContinue;
}

int Client::playCallback(const void* inputBuffer, void* outputBuffer, unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo*, PaStreamCallbackFlags, void* userData)
{
	SAMPLE* out = static_cast<SAMPLE*>(outputBuffer);
	memset(out, 0, sizeof(SAMPLE) * framesPerBuffer);
	uint64_t now = cv::getTickCount();
	auto frame = DataProcessor::getInstance().audioMixture(now, framesPerBuffer);
	std::memcpy(out, frame.data(), sizeof(SAMPLE) * framesPerBuffer);
	return paContinue;
}
