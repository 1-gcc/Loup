//  Socket.h
// (C) Copyright 2017 by Martin Brunn  see License.txt for details
//
#pragma once

#include <string>

class Socket
{
public:
	Socket();
	~Socket();
	bool create();

	bool accept(Socket& socket);
	bool listen(ushort port);
	int setSocketBlocking();
	int setSocketNonBlocking();
	bool connectTo(const char*host, ushort port);
	bool close();
	bool isOpen() const;
	bool receive(std::string & out);
	bool send(std::string & out,size_t & written);
	size_t readLine(std::string & line);
	int socket;
};

