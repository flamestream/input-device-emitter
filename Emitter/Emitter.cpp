#include "Emitter.h"
#include <format>
#include <iostream>

const std::string Emitter::DEFAULT_IP_ADDRESS = "127.0.0.1";
const unsigned short Emitter::DEFAULT_POINTER_PORT = 40610; // for-wa-ru-do
const unsigned short Emitter::DEFAULT_DIRECTINPUT_PORT = 40611;
const unsigned short Emitter::DEFAULT_KEYBOARD_PORT = 40612;
const unsigned short Emitter::DEFAULT_GAMEINPUT_PORT = 40613;

Emitter::Emitter(std::string ipAddress, unsigned short port) {

    this->broadcastAddress.sin_family = AF_INET;
    this->broadcastAddress.sin_addr.s_addr = inet_addr(ipAddress.c_str());
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

    // Initialise Winsock DLL
    // See https://beej.us/guide/bgnet/html/#windows 
    WSADATA wsaData;
    // MAKEWORD(1,1) for Winsock 1.1, MAKEWORD(2,0) for Winsock 2.0
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Failed to start Winsock: " << WSAGetLastError() << std::endl;
        return false;
    }

    // Create socket
    this->sock = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (this->sock == INVALID_SOCKET) {
        std::cerr << "Failed to create socket:" << WSAGetLastError() << std::endl;
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

    if (this == NULL) return;

    if (this->sock != INVALID_SOCKET) {
        ::closesocket(this->sock);
    }
    WSACleanup();
}

bool Emitter::send(std::string message) {

    if (this->sock == INVALID_SOCKET) {
        std::cerr << "Server must be properly setup before sending message" << std::endl;
        return false;
    }

    if (sendto(sock, message.c_str(), message.size(), 0, (sockaddr*)&this->broadcastAddress, sizeof(this->broadcastAddress)) == SOCKET_ERROR) {
        std::cerr << "Error sending message: " << WSAGetLastError() << std::endl;
        return false;
    }

    return true;
}