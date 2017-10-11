//  HttpConnection.cpp
// (C) Copyright 2017 by Martin Brunn  see License.txt for details
//
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>
#include <bsd/string.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
//#include <bits/socket_type.h>
#include <map>
#include <queue>

#include <string>
#include <errno.h>



#include "HttpConnection.h"
#include "Logger.h"
#include "Config.h"

extern Logger log;
extern Logger accessLog;

#define MAX_QUEUE 1024

HttpConnection::~HttpConnection()
{
	timeout = 5;
	socket.close();
	log.note("%d closed", socket.socket);
}


bool HttpConnection::treatRequestLine(std::string line)
{
	Func func( __PRETTY_FUNCTION__);
	if (line.empty())
	{
		
		return false;
	}
	char * method = new char[line.length() + 1];
	char * url = new char[line.length() + 1];
	char * protocol = new char[line.length() + 1];
	int ret = sscanf(line.c_str(), "%[^ \n] %[^ \n] %[^ \n]", method, url, protocol);
	if (ret < 2)
	{
		delete protocol;
		delete url;
		delete method;
		log.error("sscanf error %d", ret);
		return false;
	}
	else if (ret == 2)
	{
		strcpy(protocol, "HTTP/0.9");
	}

	header.method = method;
	header.url = url;
	header.protocol = protocol;
	delete protocol;
	delete url;
	delete method;
	return true;
}
int bind(int sockfd, const char *addr, int family)
{
	struct addrinfo hints, *res, *ressave;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = family;
	hints.ai_socktype = SOCK_STREAM;

	/* The local port it not important */
	if (getaddrinfo(addr, NULL, &hints, &res) != 0)
	{
		log.note("getaddrinfo error errno=%d", errno);
		return -1;
	}

	ressave = res;

	/* Loop through the addresses and try to bind to each */
	do {
		if (bind(sockfd, res->ai_addr, res->ai_addrlen) == 0)
			break;  /* success */
	} while ((res = res->ai_next) != NULL);

	freeaddrinfo(ressave);
	return true;
}
bool storeIn(const std::string & out)
{
	FILE * fp = fopen("downstream.trc", "ab");
	if (!fp)
		return false;
	fwrite(out.c_str(), 1, out.length(), fp);
	fprintf(fp, "\n@@@\n");
	fclose(fp);
	return true;
}
bool storeOut(const std::string & out)
{
	FILE * fp = fopen("upstream.trc", "ab");
	if (!fp)
		return false;
	fwrite(out.c_str(), 1, out.length(), fp);
	fprintf(fp, "\n@@@\n");
	fclose(fp);
	return true;
}
#define HTTP_LINE_LENGTH (1024*16)
#define MAX_BUF_SIZE (HTTP_LINE_LENGTH * 6)
size_t HttpConnection::writeQueue(std::queue<std::string> & queue,bool store)
{
	Func func(false, __func__);
	size_t ret = 0;
	log.note("writeQueue size=%d (%d)",queue.size(),socket);

	if (!queue.size())
	{
		log.note("nothing to write - stopping");
		return -1;
	}
	for (;queue.size();)
	{
		std::string & line = queue.front();
		size_t written = 0;
		if (!line.size())
		{
			log.error("empty buffer found");
			queue.pop();
			continue;
		}
		else if (!socket.send(line,written))
		{
			log.error("send returned 0");
			return 0;
		}
		else
		{
			ret = written;
			log.note("written %d / %d", written, line.size());
			if(store)
				storeOut(line.substr(0,written));
			if (written == line.size())
			{
				queue.pop();
				log.note("pop %d", socket.socket);
			}
			else
			{
				line.erase(0, written);
			}
		}
		break;
	}
	log.note("successful end write %d",ret);
	return ret;
}
size_t HttpConnection::readQueue(bool store)
{
	log.note("readQueue %d",socket);
	std::string buffer;
	if (!socket.receive(buffer))
	{
		return -1;
	}
	else if(buffer.size())
	{
		if(store)
			storeIn(buffer);
		m_queue.push(buffer);
	}
	return buffer.size();
}
bool HttpConnection::isOpen() const
{
	return socket.isOpen();
}
void HttpConnection::close()
{
	socket.close();
}
bool HttpConnection::treatHeader()
{
	std::string requestLine;
	std::string headerString;
	std::string connectionValue;
	size_t ret = socket.readLine(requestLine);
	if (ret == 0 || ret == (size_t)-1)
	{
		log.note("readline error");
		return false;
	}
	headerString = requestLine;
	if (!treatRequestLine(requestLine))
	{
		log.note("treatRequestLine error");

		return false;
	}
	std::string line;
	do
	{
		line.erase();
		if (!socket.readLine(line))
		{
			log.error("readline %d false", socket.socket);
			return false;
		}
		std::string save = line;
		std::string token;
		std::string value;
		size_t pos = line.find_first_of("\r\n");
		if (pos != std::string::npos)
		{
			line.erase(pos);

			log.note("eol found <%s>", line.c_str());
			if (!line.empty())
			{
				pos = line.find_first_of(":");
				if (pos != std::string::npos)
				{
					std::string s = line.substr(pos + 1);
					size_t pos2 = s.find_first_not_of(" \t");
					token = line.substr(0, pos);
					value = s.substr(pos2);
					headers[token] = value;

				}
				else
				{
					log.note("##error header: no colon \':\' found");
				}
			}
		}
		else
		{
		}
		if (token.empty())
		{
			headerString += save;
		}
		else
		{
			if (token != "Accept-Encoding")
				headerString += save;
			if (token == "Connection")
			{
				connectionValue = value;
			}
		}

	} while (!line.empty());
	std::string host = "[unknown]";

	if (headers.find("Host") != headers.end())
	{
		host = headers["Host"];
	}
	m_queue.push(headerString);
	log.trace("empty line found, required host <%s> <%s>\n", host.c_str(), connectionValue.c_str());
	return true;
}
bool HttpConnection::forward(HttpConnection& up,const Config & config)
{

	Func func(true,__PRETTY_FUNCTION__);
	fd_set rset, wset;
	struct timeval tv;
	time_t last_access;
	if (!treatHeader())
		return false;
	info.getConnectionInformation(socket.socket);
	std::string host = "[unknown]";

	if (headers.find("Host") != headers.end())
	{
		host = headers["Host"];
	}
	
	log.trace("[%s]<%s> %s %s %s", info.peer_name,host.c_str(),header.method.c_str(),
			header.url.c_str(), header.protocol.c_str());

	int maxfd = std::max(socket.socket, up.socket.socket);
	size_t bytes_received = 0;
	size_t total_received = 0;
	size_t total_sent = 0;
	size_t cntClient = 0;
	size_t cntServer = 0;
	bool waitingForResponse = false;
	last_access = time(NULL);
	if(socket.setSocketNonBlocking() == -1)
		return false;
	if (up.socket.setSocketNonBlocking())
		return false;
	timeout = config.timeout;
	double diff = timeout;
	bool once = true;
	do
	{
		FD_ZERO(&rset);
		FD_ZERO(&wset);
		cntClient = m_queue.size();
		cntServer = up.m_queue.size();
		tv.tv_sec = (time_t)(timeout);// -difftime(time(NULL), last_access));
		tv.tv_usec = 0;
		if (cntClient && up.isOpen())
		{
			FD_SET(up.socket.socket, &wset);
			log.note("set %d w", up.socket.socket);
		}
		if (cntServer && isOpen())
		{
			FD_SET(socket.socket, &wset);
			log.note("set %d w", socket.socket);
		}
		if (cntServer < MAX_QUEUE && up.isOpen())
		{
			FD_SET(up.socket.socket, &rset);
			log.note("set %d r", up.socket.socket);
		}
		if (cntClient < MAX_QUEUE && isOpen())
		{
			FD_SET(socket.socket, &rset);
			log.note("set %d r", socket.socket);
		}
		log.note("before select read/write %d",maxfd);
		int ret = select(maxfd + 1, &rset, &wset, NULL, &tv);
		

		if (ret == 0) {
			diff = difftime(time(NULL), last_access);
			if (diff > timeout) {
				log.trace(
					"timeout (after select) %g >= %u",
					diff, timeout);
				log.trace("%d bytes sent %d received", total_sent, total_received);
				accessLog.trace("[%s]<%s> %s %s %s (%d sent , %d recved)", info.peer_name, host.c_str(),
					header.method.c_str(), header.url.c_str(), header.protocol.c_str(), 
					total_sent, total_received);
				return false;
			}
			else {
				log.debug("after select read/write - repeat");
				continue;
			}
		}
		else if (ret < 0) {
			log.error(
				"^forward: select() error \"%s\". "
				"Closing connection (down:%d, up:%d)",
				strerror(errno), socket,
				up.socket);
			close();
			up.close();
			log.trace("%d bytes sent %d received", total_sent, total_received);
			accessLog.trace("[%s]<%s> %s %s %s (%d sent , %d recved)", info.peer_name, host.c_str(),
				header.method.c_str(), header.url.c_str(), header.protocol.c_str(), total_sent, total_received);
			return false;
		}
		else {
			log.note("after select read/write %d",ret);
			last_access = time(NULL);
		}

		if (FD_ISSET(socket.socket, &wset) && up.m_queue.size())
		{
			//log.trace("down:queue size= %d", up.m_queue.size());
			size_t sent = writeQueue(up.m_queue, false);
			//log.trace("down:bytes sent = %d", sent);
			if ((int)sent < 0)
			{
				close();
			}
		}
		if (FD_ISSET(up.socket.socket, &rset))
		{
			bytes_received =
				up.readQueue(config.storeRead);
			total_received += bytes_received;
			log.note("bytes received = %d/%d", bytes_received,total_received);
			if (bytes_received == (size_t)-1)
			{
				waitingForResponse = false;
				continue;
			}
			else if (bytes_received == 0)
			{
				waitingForResponse = false;
				continue;
			}

		}
		if (FD_ISSET(up.socket.socket, &wset))
		{
			if (!m_queue.size())
			{
				if(once)
					log.trace("end request from localhost (%d) %d bytes", up.socket.socket,total_sent);
				once = false;
			}
			else
			{
				log.note("sending queue");
				if (!total_sent)
					log.trace("begin request from localhost (%d)", up.socket.socket);
				size_t sent = up.writeQueue(m_queue, config.storeWrite);
				log.note("up bytes sent = %d", sent);
				if ((int)sent < 0)
				{
					up.close();
				}
				else if (sent > 0)
				{
					waitingForResponse = true;
					total_sent += sent;
				}
			}
		}
		if (FD_ISSET(socket.socket, &rset))
		{
			size_t recved = readQueue(false);
			log.note("bytes received = %d", recved);
			if ((int)recved < 0)
				continue;
			else if (ret == 0)
			{
			}
		}
	} while (waitingForResponse || (isOpen() && up.m_queue.size()) || (up.isOpen() && m_queue.size()));
	log.note("end loop %d %s size=%ld | %d %s size=%ld",socket,isOpen()?"open":"closed",
		m_queue.size(),
		up.socket.socket, up.isOpen() ? "open" : "closed",up.m_queue.size());
	if(isOpen())
		socket.setSocketBlocking();
	if(up.isOpen())
		up.socket.setSocketBlocking();
	log.trace("%d bytes sent %d received", total_sent, total_received);
	accessLog.trace("[%s]<%s> %s %s %s (%d sent , %d recved)", info.peer_name, host.c_str(), 
			header.method.c_str(), header.url.c_str(), header.protocol.c_str(),total_sent,total_received);
	return true;
}
const char *get_ip_string(struct sockaddr *sa, char *buf, size_t buflen)
{
	const char *result;

	buf[0] = '\0';          /* start with an empty string */

	switch (sa->sa_family) {
	case AF_INET:
	{
		struct sockaddr_in *sa_in = (struct sockaddr_in *) sa;

		result = inet_ntop(AF_INET, &sa_in->sin_addr, buf,
			(socklen_t)buflen);
		break;
	}
	case AF_INET6:
	{
		struct sockaddr_in6 *sa_in6 =
			(struct sockaddr_in6 *) sa;

		result = inet_ntop(AF_INET6, &sa_in6->sin6_addr, buf,
			(socklen_t)buflen);
		break;
	}
	default:
		/* no valid family */
		return NULL;
	}

	return result;
}

bool HttpConnection::ConnectionInformation::getConnectionInformation(int socket)
{


	struct sockaddr_storage sa;
	socklen_t salen = sizeof sa;


	/* Set the strings to default values */
	ipaddr[0] = '\0';
	strlcpy(peer_name, "<unknown host>", LENGTH_HOSTNAME);

	/* Look up the IP address */
	if (getpeername(socket, (struct sockaddr *) &sa, &salen) != 0)
		return -1;

	if (get_ip_string((struct sockaddr *) &sa, peer_ipaddr, LENGTH_IPV6) == NULL)
		return -1;

	/* Get the full host name */
	getnameinfo((struct sockaddr *) &sa, salen,
		peer_name, LENGTH_HOSTNAME, NULL, 0, 0);
	//uint32_t ui = ((struct sockaddr_in*)&sa)->sin_addr.s_addr;

	if (getsockname(socket, (struct sockaddr *) &sa, &salen) != 0)
	{
		return false;
	}
	if (get_ip_string((struct sockaddr *) &sa, ipaddr, LENGTH_IPV6) ==
		NULL)
	{
		return false;
	}
	getnameinfo((struct sockaddr *) &sa, salen,
		name, LENGTH_HOSTNAME, NULL, 0, 0);
	log.trace("peer[%s]<%s> own[%s]<%s>", peer_ipaddr, peer_name,ipaddr,name);
	return true;
}

