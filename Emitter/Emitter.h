#pragma once
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <string_view>
#pragma comment(lib, "ws2_32.lib")

class Emitter {
private:
	SOCKET sock = INVALID_SOCKET;
	sockaddr_in broadcastAddress;
public:
	const static std::string DEFAULT_IP_ADDRESS;
	const static unsigned short DEFAULT_POINTER_PORT;
	const static unsigned short DEFAULT_DIRECTINPUT_PORT;
	const static unsigned short DEFAULT_KEYBOARD_PORT;
	const static unsigned short DEFAULT_GAMEINPUT_PORT;
	const static unsigned short DEFAULT_XINPUT_PORT;

	std::string ipAddress;
	unsigned short port;

	Emitter(std::string ipAddress, unsigned short);

	std::string getIpAddress();
	unsigned short getPort();

	static bool initWinsock();
	static void cleanupWinsock();

	bool setup();
	void teardown();
	bool send(std::string_view msg);
};
