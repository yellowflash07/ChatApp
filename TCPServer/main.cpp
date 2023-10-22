#pragma once
#include "Server.h"
#include <iostream>
#include <conio.h>

int main()
{
	//create server
	Server server;
	//intiialize server
	server.Initialize();

	std::cout << "-------------------------------" << std::endl;
	std::cout << "|Press 1 to create a room      |" << std::endl;
	std::cout << "-------------------------------" << std::endl;

	while (true)
	{
		if (_kbhit())
		{
			int key = _getch();
			//create room interface
			if (key == '1')
			{
				int roomID;
				std::cout << "--CREATE ROOM--- " << std::endl;
				std::cout << "Enter room id: " << std::endl;
				std::cin >> roomID;
				server.CreateRoom(roomID);
			}
		}

		//listen to the clients
		server.ListenToClients();
	}

	server.Close();

	return 0;
}