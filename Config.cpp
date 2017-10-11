//  Config.cpp
// (C) Copyright 2017 by Martin Brunn  see License.txt for details
//
#include "Config.h"
#include "Logger.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

extern Logger log;
bool syntaxError(const char*text, size_t line)
{
	log.note("Syntax Error:<%s> on line %d\n", text, (int)line);
	return true;
}
bool syntaxError(const char*text, const char*var)
{
	log.note("Syntax Error:<%s> on line %s\n", text, var);
	return true;
}
std::string trim(const std::string&str)
{
	size_t pos = str.find_first_not_of(" \t\r\n\v");
	if (pos != std::string::npos)
	{
		size_t pos2 = str.find_last_not_of(" \t\r\n\v");
		if (pos2 == std::string::npos)
		{
			return "";
		}
		return str.substr(pos, pos2 - pos + 1);
	}
	return "";
}

std::set<std::string> getSet(const std::string & str, const char * separator)
{
	std::set<std::string> retValue;
	size_t pos = 0;
	do
	{
		size_t oldpos = pos;
		pos = str.find_first_of(separator, pos);
		std::string val;
		if (pos == std::string::npos)
			val = str.substr(oldpos);
		else
		{
			val = str.substr(oldpos, pos - oldpos);
			pos = str.find_first_not_of(separator, pos);
		}
		retValue.insert(trim(val));

	} while (pos != std::string::npos);
	return retValue;
}

Config::Options::Options()
{
	pathConfigFile = (char*)"/etc/loup/loup.conf";
}
Config::Config()
{
	ports = NULL;
	countPorts = 0;
	maxConnectionsPerProcess = UNLIMITED;
	countProcesses = 6;
	logFile = NULL;
	connectTo = NULL;
	sleepTime = 6;
	storeRead = false;
	storeWrite = false;
}

void Config::addPort(unsigned short port)
{
	countPorts++;
	if (ports == NULL)
	{
		ports = new PortInfo(port);
		return;
	}
	PortInfo * portInfo = ports;
	while (portInfo->nextPort != NULL)
	{
		if (portInfo->port == port)
		{
			countPorts--;
			return;
		}
		portInfo = portInfo->nextPort;
	}
	portInfo->nextPort = new PortInfo(port);
	return;
}
bool Config::parseLine(const char* buffer, size_t bufsize, size_t line)
{
	size_t i = 0;
	size_t beginToken = 0;
	size_t lenToken = 0;
	size_t beginValue = 0;
	size_t lenValue = 0;
	bool stop = false;
	//bool foundToken = false;
	char inString = 0;
	enum LS_State
	{
		stateLook4Token,
		stateToken,
		stateLook4Value,
		stateValue,
	} state = stateLook4Token;
	size_t lenBuffer = strlen(buffer);
	for (i = 0; i < lenBuffer && !stop; ++i)
	{
		switch (buffer[i])
		{
		case '\'':
		case '\"':
			if (inString == buffer[i])
			{
				inString = 0;
			}
			else
				inString = buffer[i];
			if (state != stateLook4Value &&
				state != stateValue)
			{
				syntaxError("Expecting token not string", line);
				exit(1);
			}
			continue;
		case '#':
			if (inString != 0)
			{
				continue;
			}
			stop = true;
			switch (state)
			{
			case stateLook4Token:
				break;
			case stateToken:
				lenToken = i - beginToken;
				break;
			case stateValue:
				break;
			default:
				syntaxError("Comment starting when expecting data", line);
				exit(1);
				break;
			}
			continue;
		case '\r':
		case ' ':
		case '\t':
		case '\v':
			switch (state)
			{
			case stateLook4Token:
			case stateLook4Value:
				break;
			case stateValue:
				lenValue++;
				break;
			case stateToken:
				state = stateLook4Value;
			default:
				break;
			}
			continue;
		default:
			switch (state)
			{
			case stateLook4Token:
				beginToken = i;
				state = stateToken;
				lenToken = 1;
				break;
			case stateToken:
				lenToken++;
				break;
			case stateLook4Value:
				beginValue = i;
				state = stateValue;
				lenValue = 1;
				//log.note( "value=%c\n", buffer[i]);
				break;
			case stateValue:
				lenValue++;
				//log.note( "valuex=%c\n", buffer[i]);
				break;
			default:
				break;
			}
		}
	}
	//log.note( "buffer=<%s> value=%ld len=%ld\n", buffer, beginValue, lenValue);
	if (!strncasecmp("logfile", &buffer[beginToken], lenToken))
	{
		logFile = new char[lenValue + 1];
		strncpy(logFile, &buffer[beginValue], lenValue);
	}
	else if (!strncasecmp("storeread", &buffer[beginToken], lenToken))
	{
		storeRead = true;
	}
	else if (!strncasecmp("storewrite", &buffer[beginToken], lenToken))
	{
		storeWrite = true;
	}
	else if (!strncasecmp("listen", &buffer[beginToken], lenToken))
	{
		char * pListen = new char[lenValue + 1];
		strncpy(pListen, &buffer[beginValue], lenValue);
		unsigned int uiPort = 0;
		// TODO test for for service names HTTP,HTTPS etc. before integer
		if (sscanf(pListen, "%u", &uiPort) != 1)
		{
			syntaxError("Cannot evaluate port at 'Listen' entry", line);
			exit(1);
		}
		delete pListen;
		addPort((unsigned short)uiPort);
	}
	else if (!strncasecmp("timeout", &buffer[beginToken], lenToken))
	{
		char * pValue = new char[lenValue + 1];
		strncpy(pValue, &buffer[beginValue], lenValue);
		unsigned int uiPort = 0;
		// TODO test for for service names HTTP,HTTPS etc. before integer
		if (sscanf(pValue, "%d", &timeout) != 1)
		{
			syntaxError("Cannot evaluate port at 'Listen' entry", line);
			exit(1);
		}
		delete pValue;
		addPort((unsigned short)uiPort);
	}
	else if (!strncasecmp("connectto", &buffer[beginToken], lenToken))
	{
		connectTo = new char[lenValue + 1];
		strncpy(connectTo, &buffer[beginValue], lenValue);
	}
	else if (!strncasecmp("connectport", &buffer[beginToken], lenToken))
	{
		connectPort = new char[lenValue + 1];
		strncpy(connectPort, &buffer[beginValue], lenValue);
	}
	else if (!strncasecmp("processcount", &buffer[beginToken], lenToken))
	{
		char * pListen = new char[lenValue + 1];
		strncpy(pListen, &buffer[beginValue], lenValue);
		int value = 0;
		if (sscanf(pListen, "%d", &value) != 1)
		{
			syntaxError("Cannot evaluate port at 'Listen' entry", line);
			exit(1);
		}
		delete pListen;
		countProcesses = value;
	}
	else if (!strncasecmp("maxconnections", &buffer[beginToken], lenToken))
	{
		char * pListen = new char[lenValue + 1];
		strncpy(pListen, &buffer[beginValue], lenValue);
		int value = 0;
		if (sscanf(pListen, "%d", &value) != 1)
		{
			syntaxError("Cannot evaluate port at 'Listen' entry", line);
			exit(1);
		}
		delete pListen;
		maxConnectionsPerProcess = value;
	}
	else if (!strncasecmp("traceflags", &buffer[beginToken], lenToken))
	{
		std::string value = std::string(&buffer[beginValue], lenValue);
		traceFlags = getSet(value, ",");
	}
	return true;
}
bool Config::load()
{
	FILE * fp = fopen(options.pathConfigFile, "r");
	if (fp)
	{
		char buffer[512];
		size_t cnt = 0;
		int line = 0;
		while ((fgets(buffer, sizeof(buffer), fp)) != NULL)
		{
			char * p = strchr(buffer, '\n');
			if (!p)
			{
				syntaxError("config:error reading", line);
				exit(1);
			}
			*p = 0;
			cnt = strlen(buffer);
			if (cnt >= sizeof(buffer))
			{
				syntaxError("config:Line too long", line);
				exit(1);
			}
			buffer[cnt] = 0;
			line++;
			parseLine(buffer, cnt, line);
		}
		fclose(fp);
	}
	else
	{
		fprintf(stdout, "config file <%s> not readable\n", options.pathConfigFile);
		log.note("config file <%s> not readable\n", options.pathConfigFile);
	}
	return true;
}
