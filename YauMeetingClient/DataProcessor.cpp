#include "DataProcessor.h"
#include"Client.h"

std::vector<std::vector<uint8_t>> DataProcessor::splitFrame(const cv::Mat& frame, std::string username, uint64_t timestamp)
{
	std::vector<uint8_t> encoded;
	std::vector<int> params = { cv::IMWRITE_JPEG_QUALITY,70 };
	cv::imencode(".jpg", frame, encoded, params);

	const size_t PACKET_SIZE = 1024, HEADER_SIZE = sizeof(TypeHeader) + sizeof(DataHeader)+username.size();
	const size_t DATA_SIZE = PACKET_SIZE - HEADER_SIZE;
	size_t totalSize = encoded.size();
	size_t totalParts = (totalSize + DATA_SIZE - 1) / DATA_SIZE;
	std::vector<std::vector<uint8_t>> packets;
	for (uint32_t i = 0; i < totalParts; i++) {
		size_t offset = i * DATA_SIZE;
		size_t datalen = std::min(DATA_SIZE, totalSize - offset);

		std::vector<uint8_t> packet(PACKET_SIZE);
		TypeHeader typeheader;
		typeheader.type = 3;

		DataHeader dataheader;
		dataheader.usernamelen = username.size();
		dataheader.datatype = 1;
		dataheader.timestamp = timestamp;
		dataheader.partid = i;
		dataheader.totalparts = totalParts;
		dataheader.data_len = datalen;
		
		std::memcpy(packet.data(), &typeheader, sizeof(TypeHeader));
		std::memcpy(packet.data() + sizeof(TypeHeader),& dataheader, sizeof(DataHeader));
		std::memcpy(packet.data() + sizeof(TypeHeader) + sizeof(DataHeader), username.data(), username.size());
		std::memcpy(packet.data() + HEADER_SIZE, encoded.data() + offset, datalen);
		
		packets.push_back(std::move(packet));
	}



	return packets;
}

std::vector<std::vector<uint8_t>> DataProcessor::splitAFrame(std::vector<SAMPLE>& frame, std::string username, uint64_t timestamp)
{
	std::lock_guard<std::mutex> lock(datamutex);

	const size_t PACKET_SIZE = 1024;
	const size_t HEADER_SIZE = sizeof(TypeHeader) + sizeof(DataHeader) + username.size();
	const size_t DATA_SIZE = (PACKET_SIZE - HEADER_SIZE) / sizeof(float) * sizeof(float);

	size_t total_bytes = std::min(frame.size() * sizeof(float), 3 * DATA_SIZE);
	size_t totalParts = (total_bytes + DATA_SIZE - 1) / DATA_SIZE;

	std::vector<uint8_t> process(total_bytes);
	memcpy(process.data(), frame.data(), total_bytes);

	size_t floats_erased = total_bytes / sizeof(float);
	frame.erase(frame.begin(), frame.begin() + floats_erased);

	std::vector<std::vector<uint8_t>> packets;
	for (size_t i = 0; i < totalParts; ++i) {
		size_t offset = i * DATA_SIZE;
		size_t len = std::min(DATA_SIZE, total_bytes - offset);

		std::vector<uint8_t> packet(PACKET_SIZE);

		TypeHeader typeheader;
		typeheader.type = 3;

		DataHeader dataheader;
		dataheader.usernamelen = username.size();
		dataheader.datatype = 2;
		dataheader.timestamp = timestamp;
		dataheader.partid = i;
		dataheader.totalparts = totalParts;
		dataheader.data_len = len;


		memcpy(packet.data(), &typeheader, sizeof(TypeHeader));
		memcpy(packet.data() + sizeof(TypeHeader), &dataheader, sizeof(DataHeader));
		memcpy(packet.data() + sizeof(TypeHeader)+ sizeof(DataHeader), username.data(), username.size());
		memcpy(packet.data() + HEADER_SIZE, process.data() + offset, len);

		packets.push_back(std::move(packet));
	}

	return packets;
}


DataProcessor::DataProcessor() {

}
DataProcessor::~DataProcessor() {

}

DataProcessor& DataProcessor::getInstance() {
	static DataProcessor instance;
	return instance;
}

void DataProcessor::videoProcess(const cv::Mat &frame,std::string username, uint64_t timestamp)
{
	auto packets = splitFrame(frame, username, timestamp);
	for (int i = 0; i < 2; i++) {

		for (const auto& packet : packets) {
			Client::getInstance().socket_send->send_to(asio::buffer(packet), Client::getInstance().roomEndpoint);
			//repeat to avoid missing
		}
	}
}
void DataProcessor::audioProcess(std::vector<SAMPLE>& aframe, std::string username, uint64_t timestamp)
{
	auto packets = splitAFrame(aframe, username, timestamp);
	std::cout << packets.size();
	for (const auto& packet : packets) {
		Client::getInstance().socket_send->send_to(asio::buffer(packet), Client::getInstance().roomEndpoint);
	}
}
cv::Mat DataProcessor::videoMixture()
{

	return cv::Mat();
}

std::vector<SAMPLE> DataProcessor::audioMixture(uint64_t now,size_t frame_size)
{
	std::vector<SAMPLE> mixed(frame_size, 0.0f);
	int active = 0;
	std::lock_guard<std::mutex> lock(audiomutex);
	for (auto &user : user_audio_buffer) {
		while (!user.second.empty() && user.second.front().first < now - 0.6*cv::getTickFrequency()) {
			user.second.pop();
		}
		if (!user.second.empty() && std::abs((int64_t)(now - user.second.front().first)) <= 0.6*cv::getTickFrequency()) {
			const auto& frame = user.second.front().second;
			for (size_t i = 0; i < frame_size && i < frame.size(); i++) {
				mixed[i] += frame[i];
			}
			active++;
		}
	}
	if (active > 0) {
		std::cout << "YES!";
		for (float& sample : mixed)sample /= active;
	}
	return mixed;
}

void DataProcessor::handlePacket(std::array<char, 1024> data)
{
	DataHeader dataheader;
	std::memcpy(&dataheader, data.data() + sizeof(TypeHeader), sizeof(DataHeader));

	char* cusername = new char[dataheader.usernamelen];
	memcpy(cusername, data.data() + sizeof(TypeHeader) + sizeof(DataHeader), dataheader.usernamelen);
	std::string username(cusername);
	delete[]cusername;

	const char* dataptr = data.data() + sizeof(TypeHeader) + sizeof(DataHeader) + username.size();
	size_t datasize = dataheader.data_len;
	uint64_t timestamp = dataheader.timestamp;
	uint32_t part = dataheader.partid;
	uint32_t total = dataheader.totalparts;
	std::cout << "___";

	if (dataheader.datatype == 1) {
		std::lock_guard<std::mutex> lock(datamutex);
		//video
		if (user_video_buffer[username][timestamp].empty() && part + 1 == total)return;

		auto& part_map = user_video_buffer[username][timestamp];
		//if (part_map.count(part) == 0) {
			part_map[part] = std::vector<char>(dataptr, dataptr + datasize);
		//}
		//std::cout << "[RECV] " << username << " frame " << timestamp/*
		//	<< " part " << part << " size=" << datasize << std::endl;*/
		if (user_video_buffer[username][timestamp].size() ==total) {

			std::vector<char> full_frame;
			for (uint32_t i = 0; i < total; i++) {
				auto& part_data = user_video_buffer[username][timestamp][i];
				full_frame.insert(full_frame.end(), part_data.begin(), part_data.end());
			}
			user_video_full[username].push(std::make_pair(timestamp, std::move(full_frame)));
			user_video_buffer[username].erase(timestamp);
			if (user_video_full[username].size() > MAX_FRAMES_PER_USER)user_video_full[username].pop();
			//std::cout << "HEEEEEEEEEEEEEEE" << std::endl;
		}
	}
	else if (dataheader.datatype == 2) {
		std::cout << "@@@";
		//audio
		std::lock_guard<std::mutex> lock(datamutex);

		const float* floatdata = reinterpret_cast<const float*>(dataptr);
		size_t float_count = datasize / sizeof(float);

		user_audio_buffer[username].push(
			std::make_pair(timestamp, std::vector<float>(floatdata, floatdata + float_count))
		);
	}
}

void DataProcessor::updateQueue()
{
	std::lock_guard<std::mutex> lock(datamutex);
	for (auto& user : user_video_full) {
		while (!user_video_full[user.first].empty() &&
			cv::getTickCount() - user_video_full[user.first].front().first > 1 * cv::getTickFrequency()) {
			user_video_full[user.first].pop();
		}
	}
	for (auto& user : user_video_buffer) {
		for (auto it = user_video_buffer[user.first].begin(); it != user_video_buffer[user.first].end();) {
			if (cv::getTickCount() - it->first > 1 * cv::getTickFrequency()) {
				it = user_video_buffer[user.first].erase(it);
			}
			else it++;
		}
	}
	for (auto it = user_audio_buffer.begin(); it != user_audio_buffer.end(); ++it) {
		while (!it->second.empty() &&
			cv::getTickCount() - it->second.front().first > 1.0 * cv::getTickFrequency()) {
			it->second.pop();
		}
	}
}

sf::Texture& DataProcessor::CV2SF(const std::vector<char> &cvdata,std::string& username)
{
	cv::Mat decodeMat = cv::imdecode(cvdata, cv::IMREAD_UNCHANGED);
	static sf::Texture emptyTexture;
	if (decodeMat.empty())return emptyTexture;
	cv::cvtColor(decodeMat, decodeMat, cv::COLOR_BGR2RGBA);
	sf::Texture &texture=user_texture_cache[username];
	if (std::abs((int32_t)texture.getSize().x - decodeMat.cols)>0.1*texture.getSize().x || 
		std::abs((int32_t)texture.getSize().y - decodeMat.rows)>0.1*texture.getSize().y) {
		texture.create(decodeMat.cols, decodeMat.rows);
	}
	texture.update(decodeMat.ptr());
	return texture;
}

const sf::Texture& DataProcessor::getFrontFrame(std::string username)
{
	updateQueue();
	std::lock_guard<std::mutex> lock(datamutex);
	static sf::Texture emptyTexture;
	if(user_video_full[username].empty())return emptyTexture;
	std::vector<char> frt = user_video_full[username].front().second;
	user_video_full[username].pop();
	return CV2SF(frt,username);
}
