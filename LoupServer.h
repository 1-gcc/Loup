//  LoupServer.h
// (C) Copyright 2017 by Martin Brunn  see License.txt for details
//
#pragma once

#ifndef LOUPSERVER_H
#define LOUPSERVER_H 1

#include <sys/types.h>
#include <stdio.h>
#include <fcntl.h>
#include <vector>
#include <string>

#include "Logger.h"
#include "SharedMemory.h"
#include "Config.h"
#include "Socket.h"
#include "Locker.h"

class LoupServer
{
public:
	LoupServer();
	~LoupServer();
	enum State
	{
		stateNull,
		stateListening,
		stateConnected,
	};
#pragma pack(push)
#pragma pack(8)
	struct StructProcess
	{
		State state;
		pid_t pid;
		size_t countConnections;

	};
#pragma pack(pop)
	class LoupProcess
	{
	protected:

	public:
		LoupProcess()
		{
		}
	};
	class SharedCounter
	{
	public:
		Locker locker;
		SharedMemory shmCounter;
		unsigned long * pCounter;
		SharedCounter()
		{
			pCounter = (unsigned long*)shmCounter.create(sizeof(*pCounter));
			*pCounter = 0;
			locker.init();
		}
		unsigned long operator++()
		{
			locker.lock_wait();
			(*pCounter)++;
			locker.lock_release();
			return *pCounter;
		}
		bool conditionalIncremenent(unsigned long max)
		{
			locker.lock_wait();
			if (*pCounter >= max)
			{
				locker.lock_release();
				return false;
			}
			(*pCounter)++;
			locker.lock_release();
			return true;
		}
		unsigned long operator--()
		{
			locker.lock_wait();
			(*pCounter)--;
			locker.lock_release();
			return *pCounter;
		}
		operator unsigned long() const
		{
			return *pCounter;
		}
	};
	class SigHandlers
	{
	public:
		typedef void SignalFunc(int);

		bool setHandlers();
		SignalFunc *setHandler(int signo, SignalFunc * func);

	};
	bool initServer(int argc, char**argv);
	bool runServer();
protected:
	Config config;
	SharedMemory processList;
	StructProcess * pProcessList;
	static bool quit;
	static bool sighupReceived;
	bool evalOptions(int argc, char**argv);
	bool createProcessPool();
	bool createProcess(size_t index);
	bool worker(StructProcess&sProcess);
	bool loop();
	//int setSocketBlocking(int sock);
	//int setSocketNonBlocking(int sock);
	static void sighupHandler(int sig);
	static Logger * plog;
	//unsigned long * pActiveCount;
	//Locker processLock;
	//SharedMemory processActiveCount;
	std::vector<Socket> vecDescriptors;
	SharedCounter activeCount;
};

#endif
