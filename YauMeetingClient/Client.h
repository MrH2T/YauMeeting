#pragma once

#include<iostream>
#include<cstring>
#include<thread>
#include<mutex>
#include<atomic>
#include<set>


#include<SFML/Main.hpp>
#include<SFML/Graphics.hpp>
#include<SFML/Window.hpp>
#include<opencv2/opencv.hpp>
#include<portaudio.h>

#include"MainFrame.h"

//these for PortAudio.
#define SAMPLE_RATE 48000
#define FRAMES_PER_BUFFER 1024
#define NUM_CHANNELS 1
typedef float SAMPLE;


class Client
{
private:

	MainFrame *frame;
	std::string hostip;
	std::string username, userpwd;
	int roomid;
	std::string roompwd;

	int tryConnect();

	std::atomic<bool> running;
	std::thread thread_toroom, thread_recv;


	std::mutex clientmutex;
	
	cv::VideoCapture cap;

	PaStream* istream,* ostream;
public:

	std::set<std::string>users;
	void stop() {
		Pa_StopStream(istream);
		Pa_StopStream(ostream);
		running = false; }

	Client();
	~Client();

	asio::io_context io_context;
	asio::ip::udp::socket* socket, *socket_send;
	asio::ip::udp::endpoint serverEndpoint, roomEndpoint;

	static Client& getInstance();
	int initClient(std::string hostip, std::string username, std::string userpwd, std::string roomid, std::string roompwd);
	void startClient();

	void videoSend();
	void audioSend();
	void videoShow();
	void audioShow();

	void handleMessage(std::array<char,1024>data);

	void updateUserList(std::string list);

	void sendLogoff();

	static int recordCallback(const void* inputBuffer, void*, unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo*,
		PaStreamCallbackFlags, void* userData);
	static int playCallback(const void* inputBuffer, void* , unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo*,
		PaStreamCallbackFlags, void* userData);
};

