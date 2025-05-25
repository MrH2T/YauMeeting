# YauMeeting

**Dependencies**

C++14; asio (non-boost), SFML, OpenCV, PortAudio.

**Structure**

YauMeetingServer/

Server.h/.cpp: Main body of server;

* RoomManager: Managing rooms.
* UserManager: Managing users online.
  * There was a fucking wrong with me, that I didn't set UserManager to singleton, causing a lot of redundant data. fuck myself.

