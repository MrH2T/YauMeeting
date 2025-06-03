#include "MainFrame.h"
#include"Client.h"
#include"DataProcessor.h"


MainFrame::MainFrame(std::string username, int roomid):window(sf::VideoMode(windowWidth, windowHeight), "YauMeeting - "+std::to_string(roomid))
{
	if (!font.loadFromFile("C:/Windows/Fonts/arial.ttf")) {//windows only
		Client::getInstance().sendLogoff();
        Client::getInstance().stop();
		exit(1);
	}
	initUI();
    selectedUser = me = username;
}

MainFrame::~MainFrame()
{
}
void MainFrame::initUI(){
	videoArea.setSize(sf::Vector2f(1000, 720));
	videoArea.setPosition(0, 0);
	videoArea.setFillColor(sf::Color(100, 100, 100));

	videoLabel.setFont(font);
	videoLabel.setString("视频画面区域");
    videoLabel.setCharacterSize(24);
    videoLabel.setFillColor(sf::Color::White);
    videoLabel.setPosition(20, 20);

   
}

void MainFrame::updateUserList(const std::vector<std::string>& users)
{
    userBoxes.clear();
    // 用户区
    const int boxWidth = 260;
    const int boxHeight = 80;
    const int userAreaX = 1020;
    const int padding = 10;

    bool flg=0;
    for (size_t i = 0; i < users.size();i++) {
        if (users[i] == selectedUser)flg = 1;
        UserBox box;
        box.username = users[i];
        box.shape.setSize(sf::Vector2f(boxWidth, boxHeight));
        box.shape.setPosition(userAreaX, 20 + i * (boxHeight + padding));
        box.shape.setFillColor(sf::Color(70, 70, 70));

        box.label.setFont(font);
        box.label.setString(users[i]);
        box.label.setCharacterSize(20);
        box.label.setFillColor(sf::Color::White);
        box.label.setPosition(userAreaX + 10, 30 + i * (boxHeight + padding));

        userBoxes.push_back(box);
    }
    if (!flg)selectedUser = me;

}
void MainFrame::run() {
    while (window.isOpen()) {
        handleEvents();
        render();
        //std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }
}
void MainFrame::handleEvents() {
    sf::Event event;
    while (window.pollEvent(event)) {
        if (event.type == sf::Event::Closed) {
            Client::getInstance().sendLogoff();
            Client::getInstance().stop();
            window.close();
        }

        if (event.type == sf::Event::MouseButtonPressed &&
            event.mouseButton.button == sf::Mouse::Left) {
            sf::Vector2f clickPos(event.mouseButton.x, event.mouseButton.y);
            for (const auto& box : userBoxes) {
                if (box.shape.getGlobalBounds().contains(clickPos)) {
                    selectedUser = box.username;
                }
            }
        }
    }
}

void MainFrame::render() {
    window.clear();
    drawVideoArea();
    drawUserList();
    window.display();
}

void MainFrame::drawVideoArea() {
    window.draw(videoArea);

    if (selectedUser.empty()) return;

    sf::Texture tex= DataProcessor::getInstance().getFrontFrame(selectedUser);
    if (tex.getSize().x != 0 || tex.getSize().y != 0) {
        frame_showing = tex;
    }
    
    sf::Sprite videoSprite(frame_showing);

    float areaW = 1000 ,areaH=windowHeight;
    float frameX = frame_showing.getSize().x, frameY = frame_showing.getSize().y;

    float scale = std::min((float)areaW / frameX, (float)areaH / frameY);
    videoSprite.setScale(scale, scale);

    float offsetX = (areaW - frameX * scale) / 2.f;
    float offsetY = (areaH - frameY * scale) / 2.f;
    videoSprite.setPosition(offsetX, offsetY);
    window.draw(videoSprite);
}
void MainFrame::drawUserList() {
    for (const auto& box : userBoxes) {
        if (box.username == selectedUser) {
            sf::RectangleShape highlight = box.shape;
            highlight.setFillColor(sf::Color(100, 100, 180));
            window.draw(highlight);
        }
        else {
            window.draw(box.shape);
        }
        window.draw(box.label);
    }
}