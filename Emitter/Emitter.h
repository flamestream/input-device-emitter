#pragma once
#include <winsock.h>
#include <string>
#pragma comment(lib, "ws2_32.lib") // Link the ws2_32.lib library

class Emitter {
private:
	SOCKET sock = INVALID_SOCKET;
	sockaddr_in broadcastAddress;
public:
	const static std::string DEFAULT_IP_ADDRESS;
	const static unsigned short DEFAULT_MOUSE_PORT;
	const static unsigned short DEFAULT_GAMEPAD_PORT;
	const static unsigned short DEFAULT_KEYBOARD_PORT;
	
	std::string ipAddress;
	unsigned short port;

	Emitter(std::string ipAddress, unsigned short);
	
	std::string getIpAddress();
	unsigned short getPort();

	bool setup();
	void teardown();
	bool send(std::string msg);
};

