#include "Buffer.h"


Buffer::~Buffer() { }

uint32_t Buffer::ReadUInt32LE()
{
	//read buffer, endian
	uint32_t value = 0;

	value |= bufferData[readIndex++];
	value |= bufferData[readIndex++] << 8;
	value |= bufferData[readIndex++] << 16;
	value |= bufferData[readIndex++] << 24;

	return value;
}

void Buffer::WriteUInt32LE(uint32_t value)
{
	//write buffer, endian
	bufferData[writeIndex++] = value;
	bufferData[writeIndex++] = value >> 8;
	bufferData[writeIndex++] = value >> 16;
	bufferData[writeIndex++] = value >> 24;
}

void Buffer::WriteString(const std::string& str)
{
	//write string buffer, string doesn't need endian
	int strLength = str.length();
	for (int i = 0; i < strLength; i++)
	{
		bufferData[writeIndex++] = str[i];
	}
}

std::string Buffer::ReadString(uint32_t length)
{
	//read string buffer, string doesn't need endian
	std::string str;
	for (int i = 0; i < length; i++)
	{
		str.push_back(bufferData[readIndex++]);
	}
	return str;
}
