#include "Server.h"
#include <TCPDebug.h>

//c'tor
Server::Server()
{
	// Initialize the sets
	FD_ZERO(&activeSockets);			
	FD_ZERO(&socketsReadyForReading);

	// Use a timeval to prevent select from waiting forever.
	tv.tv_sec = 1;
	tv.tv_usec = 0;

	//initialize other values
	info = nullptr;
	listenSocket = 0;
	bufSize = 512;
	rooms.resize(0);
}

//d'tor
Server::~Server()
{
}

void Server::Initialize()
{
	// Initialize WinSock
	WSADATA wsaData;
	int result;

	// Set version 2.2 with MAKEWORD(2,2)
	result = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (result != 0) {
		printf("WSAStartup failed with error %d\n", result);
		return;
	}
	printf("WSAStartup successfully!\n");

	struct addrinfo hints;
	ZeroMemory(&hints, sizeof(hints));	
	hints.ai_family = AF_INET;			
	hints.ai_socktype = SOCK_STREAM;	
	hints.ai_protocol = IPPROTO_TCP;	
	hints.ai_flags = AI_PASSIVE;

	result = getaddrinfo(NULL, DEFAULT_PORT, &hints, &info);
	if (result != 0) {
		printf("getaddrinfo failed with error %d\n", result);
		WSACleanup();
		return;
	}
	printf("getaddrinfo successfully!\n");

	// Create Socket
	listenSocket = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
	if (listenSocket == INVALID_SOCKET) {
		printf("socket failed with error %d\n", WSAGetLastError());
		freeaddrinfo(info);
		WSACleanup();
		return;
	}
	printf("socket created successfully!\n");

	// Bind
	result = bind(listenSocket, info->ai_addr, (int)info->ai_addrlen);
	if (TCPError(result, listenSocket, info))
	{
		return;
	}
	printf("bind was successful!\n");

	// Listen
	result = listen(listenSocket, SOMAXCONN);
	if (TCPError(result, listenSocket, info))
	{
		return;
	}
	printf("listen successful\n");
}

void Server::ListenToClients()
{
	// Reset the socketsReadyForReading
	FD_ZERO(&socketsReadyForReading);

	// Add our listenSocket to our set to check for new connections
	FD_SET(listenSocket, &socketsReadyForReading);

	// Add all of our active connections to our socketsReadyForReading
	// set.
	for (int i = 0; i < activeConnections.size(); i++)
	{
		FD_SET(activeConnections[i], &socketsReadyForReading);
	}

	//select to get connections with data concurrently
	int count = select(0, &socketsReadyForReading, NULL, NULL, &tv);
	if (count == 0)
	{
		return;
	}

	if (count == SOCKET_ERROR)
	{
		printf("select had an error %d\n", WSAGetLastError());
	}

	// Loop through socketsReadyForReading
	//   recv
	for (int i = 0; i < activeConnections.size(); i++)
	{
		//active socket
		SOCKET socket = activeConnections[i];
		if (FD_ISSET(socket, &socketsReadyForReading))
		{
			// Handle receiving data with a recv call
			//char buffer[bufSize];
			const int bufSize = 512;
			Buffer buffer(bufSize);

			//get data from the connection
			int result = recv(socket, (char*)(&buffer.bufferData[0]), bufSize, 0);
			if (result == SOCKET_ERROR)
			{
				int error = WSAGetLastError();
				//socket has disconnected/force closed. 
				if (error == WSAECONNRESET || error == WSAECONNABORTED)
				{				
					int roomID = 0;
					std::string userName;
					//erase the stored client info and socket
					for (size_t j = 0; j < clientInfoList.size(); j++)
					{
						if (clientList[i] == clientInfoList[j]->socket)
						{
							roomID = clientInfoList[j]->user.activeRoom;
							userName = clientInfoList[j]->user.userName;
							clientInfoList.erase(clientInfoList.begin() + j);				
						}
					}
					//send message to all clients in the room about the leave
					SendMessageToAllClients(roomID, userName + " has left the chat");

					printf("A client has closed connection with error: %d\n", error);

					//cleanup 
					closesocket(socket);
					FD_CLR(socket, &activeSockets);
					FD_CLR(socket, &socketsReadyForReading);
					activeConnections.erase(activeConnections.begin() + i);
					clientList.erase(clientList.begin() + i);
					continue;  // Continue processing other sockets.
				}
				else
				{
					// Handle other socket errors.
					printf("Socket error while receiving data: %d\n", error);
				}
			}
			
			if (TCPError(result, listenSocket, info))
			{
				break;
			}

			printf("Received %d bytes from the client!\n", result);

			//recieved the data, unparse it using the defined protocol
			//which is [PACKET_HEADER][MESSAGE]
			uint32_t packetSize = buffer.ReadUInt32LE();
			uint32_t messageType = buffer.ReadUInt32LE();
		
			//a regular message
			if (messageType == 1)
			{
				//get the message and the room id from the buffer
				uint32_t messageLength = buffer.ReadUInt32LE();
				std::string msg = buffer.ReadString(messageLength);
				uint32_t roomID = buffer.ReadUInt32LE();

				//send it all the clients in the room
				SendMessageToAllClients(roomID, msg);	
			}

			// joining a room
			if (messageType == 2)
			{
				//get the message and the room id from the buffer
				uint32_t messageLength = buffer.ReadUInt32LE();
				std::string userName = buffer.ReadString(messageLength);
				uint32_t roomID = buffer.ReadUInt32LE();

				bool roomFound = false;

				//find the room
				for (size_t i = 0; i < rooms.size(); i++)
				{
					//if server has this room
					if (rooms[i] == roomID)
					{
						roomFound = true;
						ClientInfo* clientInfo = new ClientInfo();
						clientInfo->user.userName = userName;
						clientInfo->socket = socket;

						//list is zero, so it's our first connection
						//no rooms to find
						//simply add the client info and break
						if (clientInfoList.size() == 0)
						{
							clientInfo->user.activeRoom = roomID;
							clientInfo->user.rooms.push_back(roomID);
							clientInfoList.push_back(clientInfo);
							std::string message = userName + " has joined the chat!";
							SendMessageToAllClients(roomID, message);
							break;
						}
						bool userIsAlreadyInRoom = false;

						//loop through the existing clients to find this client
						for (ClientInfo* client: clientInfoList)
						{
							//user already exists
							if (client->user.userName == userName)
							{
								//loop through the users room
								for (int room : client->user.rooms)
								{
									//check if user was already in the room
									if (room == roomID)
									{
										userIsAlreadyInRoom = true;
										//set the user's active room
										client->user.activeRoom = roomID;
										//send relevant message
										std::string message = userName + " has joined the chat!";
										SendMessageToAllClients(roomID, message);
										break;
									}
								}
							}

						}

						//if user didn't have the room
						if (!userIsAlreadyInRoom)
						{
							clientInfo->user.activeRoom = roomID;
							//add the room to the user
							clientInfo->user.rooms.push_back(roomID);
							clientInfoList.push_back(clientInfo);
							std::string message = userName + " has joined the chat!";
							SendMessageToAllClients(roomID, message);
							break;
						}

					}
					
				}
				if(!roomFound)
				{
					//if the server doesn't have the room
					//send an invalid message to the client
					ClientInfo* clientInfo = new ClientInfo();
					clientInfo->user.userName = userName;
					clientInfo->user.activeRoom = roomID;
					clientInfo->socket = socket;
					SendMessageToClient(clientInfo, "INVALID ROOM");
				}

			}

			// leaving a room
			if (messageType == 3)
			{
				//get the username and roomid
				uint32_t messageLength = buffer.ReadUInt32LE();
				std::string userName = buffer.ReadString(messageLength);
				uint32_t roomID = buffer.ReadUInt32LE();

				//check if the client exists, remove when found
				for (size_t j = 0; j < clientInfoList.size(); j++)
				{
					if (userName == clientInfoList[j]->user.userName)
					{
						RemoveClientFromRoom(clientInfoList[j], roomID);
					}
				}
			}

			//clear the socket, will be populated in the next loop by select
			FD_CLR(socket, &socketsReadyForReading);
			count--;
		}
	}

	// Handle any new connections
	if (count > 0)
	{
		if (FD_ISSET(listenSocket, &socketsReadyForReading))
		{
			//accept
			SOCKET newConnection = accept(listenSocket, NULL, NULL);
			if (newConnection == INVALID_SOCKET)
			{
				// Handle errors
				printf("accept failed with error: %d\n", WSAGetLastError());
			}
			else
			{
				// Handle successful connection
				clientList.push_back(newConnection);
				activeConnections.push_back(newConnection);
				FD_SET(newConnection, &activeSockets);
				FD_CLR(listenSocket, &socketsReadyForReading);

				printf("Client connected with Socket: %d\n", (int)newConnection);
			}
		}
	}
}

//create a room
void Server::CreateRoom(int roomID)
{
	rooms.push_back(roomID);
}

//send a message to all clients in a room
void Server::SendMessageToAllClients(int roomID, std::string messageToSend)
{
	for (size_t j = 0; j < clientInfoList.size(); j++)
	{
		if (roomID == clientInfoList[j]->user.activeRoom)
		{
			SendMessageToClient(clientInfoList[j], messageToSend);
		}
	}
}

//send a message to a specific client
void Server::SendMessageToClient(ClientInfo* client, std::string messageToSend)
{
	//create the buffer
	Buffer buffer(bufSize);

	//create the message
	ChatMessage chatMessage;
	chatMessage.user.activeRoom = client->user.activeRoom;
	chatMessage.message = messageToSend;
	chatMessage.messageLength = chatMessage.message.length();
	chatMessage.header.messageType = 1;
	chatMessage.header.packetSize =
		sizeof(client->user.activeRoom)
		+ chatMessage.message.length()
		+ sizeof(chatMessage.messageLength)
		+ sizeof(chatMessage.header.messageType)
		+ sizeof(chatMessage.header.packetSize);

	//write the message to the buffer
	buffer.WriteUInt32LE(chatMessage.header.packetSize);
	buffer.WriteUInt32LE(chatMessage.header.messageType);
	buffer.WriteUInt32LE(chatMessage.messageLength);
	buffer.WriteString(chatMessage.message);
	buffer.WriteUInt32LE(chatMessage.user.activeRoom);

	//send it
	int result = send(client->socket, (char*)(&buffer.bufferData[0]), bufSize, 0);
	if (TCPError(result, listenSocket, info))
	{
		return;
	}
	printf("-----Sending-----\n");
	printf("PacketSize:%d\nMessageType:%d\nMessageLength:%d\nMessage:%s\nRoomID:%d\n",
		chatMessage.header.packetSize, chatMessage.header.messageType, chatMessage.messageLength, messageToSend.c_str(), client->user.activeRoom);
}

//remove a client form a room
void Server::RemoveClientFromRoom(ClientInfo* client, int room)
{
	//loop through the client list
	for (size_t j = 0; j < clientInfoList.size(); j++)
	{
		//find the client
		if (client->socket == clientInfoList[j]->socket)
		{
			//loop through the users rooms
			for (size_t k = 0; k < clientInfoList[j]->user.rooms.size(); k++)
			{
				//find the room to leave
				if (clientInfoList[j]->user.rooms[k] == room)
				{
					//delete the room
					clientInfoList[j]->user.rooms.erase(clientInfoList[j]->user.rooms.begin() + k);
					//set user's active room to 0, will be set again when user joins again
					clientInfoList[j]->user.activeRoom = 0;
				}

				//send a message about this event to all the clients in this room.
				SendMessageToAllClients(room, client->user.userName + " has left the chat!");
			}

		}
	}
}

//close the server
void Server::Close()
{
	// Cleanup
	freeaddrinfo(info);
	closesocket(listenSocket);
	for (size_t i = 0; i < clientList.size(); i++)
	{
		closesocket(clientList[i]);
	}
	WSACleanup();
}
