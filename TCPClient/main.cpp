#pragma once

#include <iostream>
#include <string>
#include <conio.h>
#include "Client.h"

int main(int arg, char** argv)
{
	//store the user name
	std::string userName;
	std::cout << "Enter your name: ";
	std::getline(std::cin, userName);

	//create client
	Client client;

	//set username
	client.SetUserName(userName);

	//initialize
	client.Initialize();

	//help box
	std::cout << "-------------------------------" << std::endl;
	std::cout << "|Press 1 to send a message    |" << std::endl;
	std::cout << "|Press 2 to leave current room|" << std::endl;
	std::cout << "|Press 3 to join another room |" << std::endl;
	std::cout << "|Press 4 to close             |" << std::endl;
	std::cout << "-------------------------------" << std::endl;

	//run while the server is active
	while (true && client.ServerActive)
	{
		//get data from server
		client.UpdateDataFromServer();

		if (_kbhit())
		{
			int key = _getch();
			//send message interface
			if (key == '1')
			{
				std::cout << std::endl;
				std::cout << "Enter message: ";
				std::string messageToSend;

				std::getline(std::cin, messageToSend);
				client.SendAMessage(userName + ": " + messageToSend);
			}

			//leave room
			if (key == '2')
			{
				client.LeaveRoom();
			}

			//join room
			if (key == '3')
			{
				int roomID;
				std::cout << "Enter room id: ";
				std::cin >> roomID;
				client.JoinRoom(roomID);
				std::cin.ignore();
			}
			//close the client
			if (key == '4')
			{
				break;
			}
		}

	}

	client.Close();

	return 0;
}