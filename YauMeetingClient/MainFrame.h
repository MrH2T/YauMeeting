#pragma once
#include"SFML/Main.hpp"
#include"asio.hpp"
#include"SFML/Graphics.hpp"
#include"SFML/Window.hpp"
#include"opencv2/core.hpp"

struct UserBox {
	sf::RectangleShape shape;
	sf::Text label;
	std::string username;
};
class MainFrame
{
private:
	sf::Texture frame_showing;
public:
	std::string me;
	const int windowWidth = 1280;
	const int windowHeight = 720;
	sf::RenderWindow window;
	sf::Font font;
	sf::RectangleShape videoArea;
	sf::Text videoLabel;
	std::vector<UserBox> userBoxes;
	std::string selectedUser;

	MainFrame(std::string username, int roomid);
	~MainFrame();
	void run();
	void render();
	void initUI();
	void drawUserList();
	void drawVideoArea();
	void handleEvents();
	void updateUserList(const std::vector<std::string>& users);
};

