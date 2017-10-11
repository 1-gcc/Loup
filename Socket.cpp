//  Socket.cpp
// (C) Copyright 2017 by Martin Brunn  see License.txt for details
//
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/ip.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include "Socket.h"
#include "Logger.h"

#define SOL_SOCKET 1
#define SO_REUSEADDR 2
#define SO_REUSEPORT 15

#define BUFSIZE 1024
#define MAX_BUFSIZE (128 * BUFSIZE)


extern Logger log;
Socket::Socket()
{
}
bool Socket::create()
{
	socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (socket == -1)
	{
		log.error("socket error %d\n", errno);
		return false;
	}
	int on = 1;
	int ret = setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
	if (ret < 0)
	{
		log.error("setsockop error %d",errno);
		return false;
	}
	return true;
}


Socket::~Socket()
{
}
bool Socket::listen(ushort port)
{
	struct sockaddr_in addr;

	addr.sin_family = AF_INET;
	((sockaddr_in*)&addr)->sin_port = htons(port);
	((sockaddr_in*)&addr)->sin_addr.s_addr = INADDR_ANY;
	int ret = -1;
	ret = bind(socket, (const sockaddr*)&addr, sizeof(addr));
	if (ret == -1)
	{
		log.error("bind error %d\n", errno);
		exit(2);
		return false;
	}
	ret = ::listen(socket, SOMAXCONN);
	if (ret == -1)
	{
		log.error("listen error %d\n", errno);
		return false;
	}
	log.trace("Listen on port %d\n", port);

	return true;
}

bool Socket::accept(Socket& listenSocket)
{
	struct sockaddr_in addr;
	int retVal = listenSocket.setSocketBlocking();
	if (retVal == -1)
	{
		log.error("setSocketBlocking return != -1");
		return false;
	}
	socklen_t len = sizeof(addr);
	log.note("before accept listening\n");
	socket = ::accept(listenSocket.socket, (sockaddr*)&addr, &len);
	int _errno = errno;
	log.note("after accept listening %d\n", socket);

	if (socket < 0)
	{
		log.error("error on accept %d <%s>\n", _errno, strerror(_errno));
		return false;
	}

	return true;
}
bool Socket::close()
{
	log.note("closing %d", socket);
	if(socket != -1)
		::close(socket);
	socket = -1;
	return true;
}
bool Socket::isOpen() const
{
	if (socket != -1)
		return true;
	return false;

}
bool Socket::connectTo(const char*host, ushort port)
{
	if (!create())
		return false;
	struct addrinfo hints;
	struct addrinfo *pAddr;
	struct addrinfo *pAddrSave;
	char portstr[6];
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	snprintf(portstr, sizeof(portstr), "%d", port);

	int n = getaddrinfo(host, portstr, &hints, &pAddr);
	if (n != 0) {
		log.note(
			"connectTo: Could not get addr info for <%s>", host);
		return -1;
	}

	log.note(
		"connectTo: getaddrinfo returned for <%s:%d>", host, port);

	pAddrSave = pAddr;
	//int connectSocket = -1;
	do
	{

		/*connectSocket = ::socket(pAddr->ai_family, pAddr->ai_socktype, pAddr->ai_protocol);
		if (connectSocket < 0)
			continue;*/

		if (connect(socket, pAddr->ai_addr, pAddr->ai_addrlen) == 0)
			break;

		close();
		
	} while ((pAddr = pAddr->ai_next) != NULL);

	freeaddrinfo(pAddrSave);
	log.note("connectTo socket %d", socket);
	return socket != -1 ? true : false;

}
bool Socket::receive(std::string & out)
{
	char * buffer = new char[MAX_BUFSIZE];
	size_t ret = recv(socket, buffer, MAX_BUFSIZE, 0);
	if (ret == (size_t)-1)
	{
		switch (errno)
		{
#ifdef EWOULDBLOCK
		case EWOULDBLOCK:
#else
#  ifdef EAGAIN
		case EAGAIN:
#  endif
#endif
		case EINTR:
			log.note("EINTR stops readQueue");
			ret = 0;
			break;
		default:
			log.note(
				"readQueue: read() failed on fd %d: %s",
				socket, strerror(errno));
			ret = -1;
			break;
		}


	}
	else if (ret == 0)
	{
		close();
	}
	else
	{
		out = std::string(buffer, ret);
	}
	delete buffer;
	return ret != (size_t)-1 ? true : false;
}
bool Socket::send(std::string & out,size_t & written)
{
	size_t ret = ::send(socket, out.c_str(), out.size(), MSG_NOSIGNAL);
	if (ret == (size_t)-1)
	{
		switch (errno)
		{
#ifdef EWOULDBLOCK
		case EWOULDBLOCK:
#else
#  ifdef EAGAIN
		case EAGAIN:
#  endif
#endif
		case EINTR:
			return false;
		case ENOBUFS:
		case ENOMEM:
			log.note(
				"send() error [NOBUFS/NOMEM] \"%s\" on "
				"file descriptor %d", strerror(errno),
				socket);
			return false;
		default:
			log.note("send (%d) error errno=%d", socket, errno);
			return false;
		}
	}
	else if (ret < out.size())
	{
		if (ret > 0)
		{
		}
		else
		{
			log.error("send (%d) returned 0", socket);
		}
	}
	written = ret;
	return true;
}
int Socket::setSocketBlocking()
{
	Func func(__PRETTY_FUNCTION__);
	int flags = fcntl(socket, F_GETFL, 0);
	if (flags == -1)
	{
		log.error("error setSocketBlocking (%d) %d", socket, errno);
		return -1;
	}
	flags = fcntl(socket, F_SETFL, flags & ~O_NONBLOCK);
	if (flags == -1)
	{
		log.error("error setSocketBlocking (%d) %d", socket, errno);
		return -1;
	}
	else
		log.note("setSocketBlocking (%d)", socket);
	return flags;
}
int Socket::setSocketNonBlocking()
{
	Func func(__PRETTY_FUNCTION__);
	int flags = fcntl(socket, F_GETFL, 0);
	if (flags == -1)
	{
		log.error("error setSocketNonBlocking (%d) %d", socket, errno);
		return -1;
	}
	flags = fcntl(socket, F_SETFL, flags | O_NONBLOCK);
	if (flags == -1)
	{
		log.error("error setSocketNonBlocking (%d) %d", socket, errno);
		return -1;
	}
	else
		log.note("setSocketNonBlocking %d", socket);
	return flags;
}
size_t Socket::readLine(std::string & line)
{
	Func func(__PRETTY_FUNCTION__);
	char buffer[BUFSIZE] = "";
	size_t allRead = 0;
	for (;;)
	{
		long int readChars = 0;
		size_t ret = recv(socket, buffer, BUFSIZE, MSG_PEEK);
		log.note("recv (%d) peek %d bytes", socket, ret);
		if (ret == 0)
			return ret;
		if (ret == (size_t)-1)
		{
			log.error("recv error %d", errno);
			return ret;
		}

		char * p = (char *)memchr(buffer, '\n', ret);
		if (p)
			readChars = p - buffer + 1;
		else
			readChars = ret;

		line += std::string(buffer, readChars);

		allRead += readChars;
		ret = recv(socket, buffer, readChars, 0);
		if (ret == (size_t)-1)
		{
			log.error("recv returned -1 %d", errno);
			break;
		}
		log.note("recv (%d) %d bytes total %d bytes", socket, ret, allRead);
		log.note("<%s>", std::string(buffer, ret).c_str());
		if (p)
			break;
	}

	return allRead;
}