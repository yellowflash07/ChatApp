#pragma once

// WinSock2 Windows Sockets
#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

#include <vector>
#include <string>
#include "buffer.h"
#include <map>

// Need to link Ws2_32.lib
#pragma comment(lib, "Ws2_32.lib")
#define DEFAULT_PORT "8412"

struct ClientInfo
{
	User user;
	SOCKET socket;
};

class Server
{
public:
	Server();
	~Server();

	void Initialize();
	void ListenToClients();
	void CreateRoom(int roomID);
	void SendMessageToAllClients(int roomID, std::string messageToSend);
	void SendMessageToClient(ClientInfo* client, std::string messageToSend);
	void RemoveClientFromRoom(ClientInfo* client, int room);
	void Close();
private:
	timeval tv;
	std::vector<int> rooms;
	std::vector<SOCKET> activeConnections;
	std::vector<SOCKET> clientList;
	std::vector<ClientInfo*> clientInfoList;
	FD_SET activeSockets;				// List of all of the clients ready to read.
	FD_SET socketsReadyForReading;		// List of all of the connections
	struct addrinfo* info;
	SOCKET listenSocket;
	int bufSize;

};

