#pragma once
#pragma once

#include <vector>
#include <string>

//to hold information about a user
struct User
{
	std::string userName;
	std::vector<int> rooms;
	int activeRoom;
};

//packet information
struct PacketHeader
{
	uint32_t packetSize;
	uint32_t messageType;
};

//message
struct ChatMessage
{
	PacketHeader header;
	uint32_t messageLength;
	User user;
	std::string message;
};


//buffer
class Buffer
{
public:
	Buffer(int size = 512) 
	{
		bufferData.resize(size);
		writeIndex = 0;
		readIndex = 0;
	};
	~Buffer();

	//read the next item in the buffer
	uint32_t ReadUInt32LE(); 

	//write int to the buffer
	void WriteUInt32LE(uint32_t value);

	//write string to the buffer
	void WriteString(const std::string& str);

	//read string from buffer
	std::string ReadString(uint32_t length);

	//buffer data
	std::vector<uint8_t> bufferData;

	//write index
	int writeIndex;

	//read index
	int readIndex;
};