#pragma once
// WinSock2 Windows Sockets
#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <WinSock2.h>
#include <WS2tcpip.h>

bool CheckTCPError(int result, SOCKET socket, addrinfo* info, const char* file, int line);
#define TCPError(result, socket, info) CheckTCPError(result, socket, info, __FILE__, __LINE__)