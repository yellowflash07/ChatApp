#include "TCPDebug.h"
#include <stdio.h>

//check tcp errors, release socket, free address on fail
bool CheckTCPError(int result, SOCKET socket, addrinfo* info, const char* file, int line)
{
	if (result == SOCKET_ERROR)
	{
		printf("TCP Error [%d]: '%s' at %d\n", WSAGetLastError(), file, line);
		closesocket(socket);
		freeaddrinfo(info);
		WSACleanup();
		return true;
	}
	else
	{
		return false;
	}
}