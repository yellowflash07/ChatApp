#include "Client.h"
#include <TCPDebug.h>
#include <iostream>

#define DEFAULT_PORT "8412"

//c'tor
Client::Client()
{
	//time val init
	tv.tv_sec = 1;
	tv.tv_usec = 0;
	//other values
	info = nullptr;
	serverSocket = 0;
	ServerActive = false;
	//reset sets
	FD_ZERO(&serverReadyForReading);
	FD_SET(serverSocket, &serverReadyForReading);
}

//d'tor
Client::~Client()
{
}

void Client::Initialize()
{
	// Initialize WinSock
	WSADATA wsaData;
	int result;

	// Set version 2.2 with MAKEWORD(2,2)
	result = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (result != 0) {
		std::cout << ("WSAStartup failed with error %d\n", result) << std::endl;
		return;
	}
	printf("WSAStartup successfully!\n");

	info = nullptr;
	struct addrinfo hints;
	ZeroMemory(&hints, sizeof(hints));	
	hints.ai_family = AF_INET;			
	hints.ai_socktype = SOCK_STREAM;	
	hints.ai_protocol = IPPROTO_TCP;	
	hints.ai_flags = AI_PASSIVE;


	result = getaddrinfo("127.0.0.1", DEFAULT_PORT, &hints, &info);
	if (result != 0) {
		printf("getaddrinfo failed with error %d\n", result);
		WSACleanup();
		return;
	}
	printf("getaddrinfo successfully!\n");

	// Socket
	serverSocket = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
	if (serverSocket == INVALID_SOCKET) {
		printf("socket failed with error %d\n", WSAGetLastError());
		freeaddrinfo(info);
		WSACleanup();
		return;
	}
	printf("socket created successfully!\n");

	// Connect
	result = connect(serverSocket, info->ai_addr, (int)info->ai_addrlen);
	if (serverSocket == INVALID_SOCKET) {
		printf("connect failed with error %d\n", WSAGetLastError());
		closesocket(serverSocket);
		freeaddrinfo(info);
		WSACleanup();
		return;
	}
	printf("Connect to the server successfully!\n");

	ServerActive = true;
	FD_ZERO(&serverReadyForReading);
}

//send a message to the server
void Client::SendAMessage(std::string messageToSend)
{
	//if not in a room, return
	if (currentRoom.roomID == 0)
	{
		std::cout << "You are not in a room, please join a room!" << std::endl;
		return;
	}

	//create the message
	ChatMessage message;
	message.user.activeRoom = currentRoom.roomID;
	message.message = messageToSend;
	message.messageLength = messageToSend.length();
	message.header.messageType = 1;
	message.header.packetSize =
		  sizeof(currentRoom.roomID)
		+ message.message.length()
		+ sizeof(message.messageLength)			
		+ sizeof(message.header.messageType)	
		+ sizeof(message.header.packetSize);	

	//create the buffer
	const int bufSize = 512;
	Buffer buffer(bufSize);

	//write the buffer
	buffer.WriteUInt32LE(message.header.packetSize);
	buffer.WriteUInt32LE(message.header.messageType);	
	buffer.WriteUInt32LE(message.messageLength);		
	buffer.WriteString(message.message);				
	buffer.WriteUInt32LE(currentRoom.roomID);

	// send it to server
	int result = send(serverSocket, (const char*)(&buffer.bufferData[0]), message.header.packetSize, 0);
	if (TCPError(result, serverSocket, info))
	{
		return;
	}
}

//set the client's user name
void Client::SetUserName(std::string name)
{
	user.userName = name;
}

//join a room, returns false when the server doesn't have the room
bool Client::JoinRoom(int roomID)
{
	if (currentRoom.roomID == roomID)
	{
		printf("Already in the room!\n");
		return false;
	}


	//create the message
	ChatMessage message;
	//write the requested room id
	message.user.activeRoom = roomID;
	//with the username
	message.message = user.userName;
	message.messageLength = message.message.length();
	//message type is 2 for the server to recongize this as a join request
	message.header.messageType = 2;
	message.header.packetSize =
		sizeof(message.user.activeRoom)
		+ message.message.length()
		+ sizeof(message.messageLength)
		+ sizeof(message.header.messageType)
		+ sizeof(message.header.packetSize);

	const int bufSize = 512;
	Buffer buffer(bufSize);

	buffer.WriteUInt32LE(message.header.packetSize);
	buffer.WriteUInt32LE(message.header.messageType);
	buffer.WriteUInt32LE(message.messageLength);
	buffer.WriteString(message.message);
	buffer.WriteUInt32LE(roomID);

	// send the request to the server
	int result = send(serverSocket, (const char*)(&buffer.bufferData[0]), message.header.packetSize, 0);
	if (TCPError(result, serverSocket, info))
	{
		return false;
	}

	//init a buffer to store the result
	Buffer resultBuffer(bufSize);

	//server should have sent a message back about the request
	result = recv(serverSocket, (char*)(&resultBuffer.bufferData[0]), bufSize, 0);
	if (TCPError(result, serverSocket, info))
	{
		return false;
	}
	//parse the buffer
	uint32_t packetSize = resultBuffer.ReadUInt32LE();
	uint32_t messageType = resultBuffer.ReadUInt32LE();

	uint32_t messageLength = resultBuffer.ReadUInt32LE();
	std::string msg = resultBuffer.ReadString(messageLength);
	int recievedRoomID = resultBuffer.ReadUInt32LE();


	//if the message is INVALID ROOM, server didn't have the room
	//and refused connection
	//terminate with return value of false
	if (msg == "INVALID ROOM")
	{
		std::cout << "Invalid room id!" << std::endl;
		return false;
	}
	
	if (currentRoom.roomID != 0)
	{
		SendAMessage(user.userName + " has left the chat!");
	}

	//set the room
	user.activeRoom = roomID;

	//add the room info
	if (roomInfoList.size() > 0)
	{
		for (RoomInfo roomInfo : roomInfoList)
		{
			if (roomInfo.roomID == roomID)
			{
				currentRoom = roomInfo;
				for (size_t i = 0; i < roomInfo.messages.size(); i++)
				{
					//INCOMPLETE IMPLEMENTATION
					//tried to output the history of the messages
					//when a client joins back
					//but this doesnt work because i am deleting the room
					//when the client leaves 
					printf(roomInfo.messages[i].c_str());
				}
				break;
			}

			RoomInfo newRoomInfo;
			newRoomInfo.roomID = roomID;
			roomInfoList.push_back(newRoomInfo);
			currentRoom = newRoomInfo;
		}
	}
	else
	{
		//create the new room and store it
		RoomInfo newRoomInfo;
		newRoomInfo.roomID = roomID;
		roomInfoList.push_back(newRoomInfo);
		currentRoom = newRoomInfo;
	}
	printf("You have joined the chat!\n");

	return true;
}

//leave the room
void Client::LeaveRoom()
{
	//if client is not in a room, return
	if (currentRoom.roomID == 0)
	{
		printf("You are not in a room, please join a room!\n");
		return;
	}
	ChatMessage message;
	message.user.activeRoom = currentRoom.roomID;
	message.message = user.userName;
	message.messageLength = message.message.length();
	//set message type to 3 to let the server know this client is leaving
	message.header.messageType = 3;
	message.header.packetSize =
		sizeof(user.activeRoom)
		+ message.message.length()
		+ sizeof(message.messageLength)
		+ sizeof(message.header.messageType)
		+ sizeof(message.header.packetSize);


	const int bufSize = 512;
	Buffer buffer(bufSize);

	buffer.WriteUInt32LE(message.header.packetSize);
	buffer.WriteUInt32LE(message.header.messageType);
	buffer.WriteUInt32LE(message.messageLength);
	buffer.WriteString(message.message);
	buffer.WriteUInt32LE(message.user.activeRoom);

	//send the left message to server
	int result = send(serverSocket, (const char*)(&buffer.bufferData[0]), message.header.packetSize, 0);
	if (TCPError(result, serverSocket, info))
	{
		return;
	}

	currentRoom.roomID = 0;
	printf("Successfully left the chat, press 3 to join a room! \n");
}

//close the client
void Client::Close()
{
	// Close
	printf("Client closed! \n");
	freeaddrinfo(info);
	closesocket(serverSocket);
	WSACleanup();
}

//update data from server when it has one
void Client::UpdateDataFromServer()
{
	// Reset the socketsReadyForReading
	FD_ZERO(&serverReadyForReading);
	//add our serversocket to the readlist
	FD_SET(serverSocket, &serverReadyForReading);

	//check if the server has anything to say, select for concurrency
	int count = select(0, &serverReadyForReading, NULL, NULL, &tv);
	if (count > 0)
	{
		//server has a message

		//recieve the message
		const int bufSize = 512;
		Buffer buffer(bufSize);
		int result = recv(serverSocket, (char*)(&buffer.bufferData[0]), bufSize, 0);

		if (result == SOCKET_ERROR)
		{
			//if error, server closed. close the client.
			int error = WSAGetLastError();
			if (error == WSAECONNRESET || error == WSAECONNABORTED)
			{
				ServerActive = false;
				printf("Server has closed: %d\n", error);
			}
			else
			{
				// Handle other socket errors.
				printf("Socket error while receiving data: %d\n", error);
				// Handle these errors as appropriate for your application.
			}
		}
		if (TCPError(result, serverSocket, info))
		{
			return;
		}

		//unpack
		uint32_t packetSize = buffer.ReadUInt32LE();
		uint32_t messageType = buffer.ReadUInt32LE();

		//server sent us a message from a different client
		if (messageType == 1)
		{
			//print the message out to the screen
			uint32_t messageLength = buffer.ReadUInt32LE();
			std::string msg = buffer.ReadString(messageLength);
			int roomID = buffer.ReadUInt32LE();
			if (roomID != this->currentRoom.roomID) return;
			std::cout << msg.c_str() << std::endl;
		}
		//clear the socket for population in the next loop
		FD_CLR(serverSocket, &serverReadyForReading);
	}
}
