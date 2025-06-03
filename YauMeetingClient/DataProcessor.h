#pragma once

#include<opencv2/opencv.hpp>
#include<vector>
#include<map>
#include<queue>
#include<SFML/Main.hpp>
#include<SFML/Graphics.hpp>
#include<portaudio.h>

#include"Header.h"


typedef float SAMPLE;
class DataProcessor
{
private:
	std::vector<std::vector<uint8_t>> splitFrame(const cv::Mat& frame,std::string username, uint64_t timestamp);
	std::vector<std::vector<uint8_t>> splitAFrame(std::vector<SAMPLE>& frame, std::string username, uint64_t timestamp);

	std::map < std::string, std::map<uint64_t, std::map<uint32_t,std::vector<char>>>> user_video_buffer;
	std::map < std::string, std::queue<std::pair<uint64_t, std::vector<char>>>> user_video_full;
	std::map<std::string, std::queue<std::pair<uint64_t,std::vector<SAMPLE>>>> user_audio_buffer;

	std::map<std::string, sf::Texture> user_texture_cache;
public:
	const int MAX_FRAMES_PER_USER = 7;
	std::mutex datamutex,audiomutex;
	std::vector<SAMPLE> audioBuffer;
	DataProcessor();
	~DataProcessor();
	static DataProcessor& getInstance();

	void videoProcess(const cv::Mat& frame, std::string username, uint64_t timestamp);
	void audioProcess(std::vector<SAMPLE>& aframe, std::string username, uint64_t timestamp);

	cv::Mat videoMixture();
	std::vector<SAMPLE> audioMixture(uint64_t now,size_t frame_size);

	void handlePacket(std::array<char,1024> data);

	void updateQueue();

	sf::Texture& CV2SF(const std::vector<char>& cvdata,std::string &username);

	const sf::Texture& getFrontFrame(std::string username);
};

