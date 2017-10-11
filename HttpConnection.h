//  HttpConnection.h
// (C) Copyright 2017 by Martin Brunn  see License.txt for details
//
#pragma once

#include <string.h>
#include <string>
#include <map>
#include <queue>

#include "Socket.h"

#define LENGTH_IPV6		48
#define LENGTH_HOSTNAME		1024

class Config;

class HttpConnection
{
public:
	typedef std::map<std::string, std::string> Headers;
	class ConnectionInformation
	{
	public:
		bool getConnectionInformation(int socket);
		char peer_ipaddr[LENGTH_IPV6 + 1];
		char peer_name[LENGTH_HOSTNAME];
		char name[LENGTH_HOSTNAME];
		char ipaddr[LENGTH_IPV6 + 1];
		ConnectionInformation()
		{
			memset(peer_ipaddr, 0, sizeof(peer_ipaddr));
			memset(peer_name, 0, sizeof(peer_name));
			memset(name, 0, sizeof(name));
			memset(ipaddr, 0, sizeof(ipaddr));
		}
	};
	class HttpHeader
	{
	public:
		std::string method;
		std::string url;
		std::string protocol;
		std::string server;
		std::string content_length;
		int contentLength;
		std::string ipaddress;
		std::string name;
		Headers headers;
		bool readRequest(const char*line)
		{

			return true;
		}
		bool readHeader(const char*line)
		{
			return true;
		}
	};
	HttpConnection(Socket & socket) : socket(socket)
	{
		timeout = 5;
		//info.getConnectionInformation(m_socket);
	}
	~HttpConnection();
	//static int connectTo(const char*host, int port);
	bool treatRequestLine(std::string line);
	bool treatHeader();

	bool forward(HttpConnection&up, const Config & config);
	size_t readQueue(bool store);
	size_t writeQueue(std::queue<std::string> & queue, bool store);
	void close();
	bool isOpen() const;
public:
	ConnectionInformation info;
	Headers headers;
	HttpHeader header;
	std::queue<std::string> m_queue;
	Socket socket;
	int timeout;
	size_t contentLength;
};

