//  Config.h
// (C) Copyright 2017 by Martin Brunn  see License.txt for details
//
#pragma once

#include <set>
#include <string>

class Config
{
public:
	class Options
	{
	public:
		char * pathConfigFile;
		Options();

	};
	class PortInfo
	{
	public:
		unsigned short port;
		PortInfo * nextPort;
		PortInfo()
		{
			port = 0;
			nextPort = NULL;
		}
		PortInfo(unsigned short port)
		{
			this->port = port;
			nextPort = NULL;
		}
	};
	static const int UNLIMITED = 0;
	size_t countProcesses;
	size_t maxConnectionsPerProcess;
	size_t countPorts;
	size_t sleepTime;
	int timeout;
	char * logFile;
	char * connectTo;
	char * connectPort;
	std::set<std::string> traceFlags;
	PortInfo * ports;
	Options options;
	bool storeWrite;
	bool storeRead;
	Config();
	bool load();
protected:
	bool parseLine(const char* buffer, size_t bufsize, size_t line);
	void addPort(unsigned short port);
};

