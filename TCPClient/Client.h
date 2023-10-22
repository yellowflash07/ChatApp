#pragma once

#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <conio.h>
#include <vector>
#include "buffer.h"

#pragma comment(lib, "Ws2_32.lib")

struct RoomInfo
{
	int roomID;
	std::vector<std::string> messages;
};

class Client
{
public:
	Client();
	~Client();

	void Initialize(); 
	void SendAMessage(std::string message);
	void SetUserName(std::string name);
	bool JoinRoom(int roomID);
	void LeaveRoom();
	void UpdateDataFromServer();
	void Close();
	//is the server active?
	bool ServerActive;

private:
	SOCKET serverSocket;
	//set with only the server to use the non-blocking server call
	FD_SET serverReadyForReading;		
	struct addrinfo* info;
	//client user info
	User user;
	//time value
	timeval tv;
	//list of this clients rooms
	std::vector<RoomInfo> roomInfoList;
	//info of the clients current room
	RoomInfo currentRoom;
};
