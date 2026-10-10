#include "Emitter.h"
#include <format>
#include <iostream>

const std::string Emitter::DEFAULT_IP_ADDRESS = "127.0.0.1";
const unsigned short Emitter::DEFAULT_POINTER_PORT = 40610; // for-wa-ru-do
const unsigned short Emitter::DEFAULT_DIRECTINPUT_PORT = 40611;
const unsigned short Emitter::DEFAULT_KEYBOARD_PORT = 40612;
const unsigned short Emitter::DEFAULT_GAMEINPUT_PORT = 40613;
const unsigned short Emitter::DEFAULT_XINPUT_PORT = 40614;

bool Emitter::initWinsock() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Failed to start Winsock: " << WSAGetLastError() << std::endl;
        return false;
    }
    return true;
}

void Emitter::cleanupWinsock() {
    WSACleanup();
}

Emitter::Emitter(std::string ipAddress, unsigned short port) {
    ZeroMemory(&this->broadcastAddress, sizeof(this->broadcastAddress));
    this->broadcastAddress.sin_family = AF_INET;
    InetPtonA(AF_INET, ipAddress.c_str(), &this->broadcastAddress.sin_addr);
    this->broadcastAddress.sin_port = htons(port);
    this->ipAddress = ipAddress;
    this->port = port;
}

std::string Emitter::getIpAddress() {
    return this->ipAddress;
}

unsigned short Emitter::getPort() {
    return this->port;
}

bool Emitter::setup() {
    // Create socket
    this->sock = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (this->sock == INVALID_SOCKET) {
        std::cerr << "Failed to create socket: " << WSAGetLastError() << std::endl;
        return false;
    }

    // Set socket to broadcast mode
    int enabled = 1;
    if (setsockopt(this->sock, SOL_SOCKET, SO_BROADCAST, (char*)&enabled, sizeof(enabled)) == SOCKET_ERROR) {
        std::cerr << "Failed to configure socket: " << WSAGetLastError() << std::endl;
        return false;
    }

    return true;
}

void Emitter::teardown() {
    if (this->sock != INVALID_SOCKET) {
        ::closesocket(this->sock);
        this->sock = INVALID_SOCKET;
    }
}

bool Emitter::send(std::string_view message) {
    if (this->sock == INVALID_SOCKET) {
        std::cerr << "Server must be properly setup before sending message" << std::endl;
        return false;
    }

    if (sendto(sock, message.data(), static_cast<int>(message.size()), 0, (sockaddr*)&this->broadcastAddress, sizeof(this->broadcastAddress)) == SOCKET_ERROR) {
        std::cerr << "Error sending message: " << WSAGetLastError() << std::endl;
        return false;
    }

    return true;
}
