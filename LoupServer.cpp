//  LoupServer.cpp
// (C) Copyright 2017 by Martin Brunn  see License.txt for details
//
// (C) 2017 Copyright Martin Brunn
//
// https://www.w3.org/Protocols/rfc2616/rfc2616-sec4.html#sec4.2
//
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>


#include <vector>

#include "LoupServer.h"
#include "SharedMemory.h"
#include "HttpConnection.h"
#include "Socket.h"

extern Logger log;
extern Logger accessLog;
bool LoupServer::sighupReceived = false;
bool LoupServer::quit = false;
Logger * LoupServer::plog = NULL;

LoupServer::LoupServer()
{
	quit = false;
	sighupReceived = false;
	plog = &log;
}


LoupServer::~LoupServer()
{
}

bool LoupServer::evalOptions(int argc, char**argv)
{
	log.note("evalOptions()");
	while (--argc)
	{
		char * arg = *++argv;
		if (strlen(arg) < 2)
			continue;
		switch (arg[0])
		{
		case '-':
			switch (arg[1])
			{
			case 'c':
			case 'C':
				arg = *++argv;
				config.options.pathConfigFile = arg;
				break;
			default:
				fprintf(stderr,"Unknown option %s", arg);
			}
			break;
		}
	}
	return true;
}
bool LoupServer::initServer(int argc, char**argv)
{
	evalOptions(argc, argv);
	config.load();
	if (!log.load(config.traceFlags))
	{
		fprintf(stderr, "Cannot load logger.so\n");
		return false;
	}
	accessLog.load(config.traceFlags);
	accessLog.setTraceFile("Access_20170929.log");
	log.trace("initServer %d:%d",getpid(),getppid());
	
	return true;
}
bool LoupServer::runServer()
{
	Func func(true,__PRETTY_FUNCTION__);
	if (!createProcessPool())
		return false;
	loop();
	return true;
}

bool LoupServer::createProcessPool()
{
	Func func(true,__PRETTY_FUNCTION__);

	pProcessList = (StructProcess*)
		processList.create(sizeof(StructProcess) * config.countProcesses);
	for (size_t i = 0; i < config.countProcesses; ++i)
	{
		pProcessList[i].countConnections = 0;
		pProcessList[i].pid = 0;
		pProcessList[i].state = stateNull;
	}
	Socket listen;
	if (!listen.create())
		return false;
	if (!listen.listen(config.ports->port))
		return false;
	vecDescriptors.push_back(listen);
	for (size_t i = 0; i < config.countProcesses; ++i)
	{
		if (createProcess(i))
		{
			++activeCount;
			//listen.close();
		}
	}
	return true;
}
bool LoupServer::createProcess(size_t index)
{
	Func func(true,__PRETTY_FUNCTION__);
	pid_t pid = fork();
	if (pid == 0)
	{
		return worker(pProcessList[index]);
	}
	pProcessList[index].pid = pid;
	pProcessList[index].state = stateListening;

	return true;
}

bool LoupServer::worker(StructProcess&sProcess)
{
	Func func(true,__PRETTY_FUNCTION__);
	int max_socket = 0;
	
	int retVal = 0;
	fd_set fdset;

	
	while (!quit)
	{
		FD_ZERO(&fdset);
		for (size_t i = 0; i < vecDescriptors.size(); i++)
		{
			Socket & socket = vecDescriptors[i];
			if (socket.setSocketNonBlocking() == -1)
				continue;
			FD_SET(socket.socket, &fdset);
			max_socket = std::max(max_socket, socket.socket);
			
		}
		if (max_socket == 0)
			return false;
		sProcess.state = stateListening;
		log.note( "before select listening\n");
		retVal = select(max_socket + 1, &fdset, NULL, NULL, NULL);

		if (retVal == -1)
		{
			if (EINTR == errno)
			{
				log.note( "select error EINTR %d\n", errno);
				continue;
			}
			log.error( "select error %d\n", errno);
			return false;
		}
		else if (retVal == 0)
		{
			log.note("error select 0");
			continue;
		}
		log.note( "after select listening\n");
		bool found = false;
		Socket * pListenSocket = NULL;
		for (size_t i = 0; i < vecDescriptors.size(); ++i)
		{
			Socket & socket = vecDescriptors[i];
			if (FD_ISSET(socket.socket, &fdset))
			{
				pListenSocket = &socket;
				log.note("socket %d is set\n",pListenSocket->socket);
				found = true;
				break;
			}
		}
		if (!found)
		{
			log.error( "no listen socket was readable after call to select\n");

			continue;
		}
		Socket socket;
		if (!socket.accept(*pListenSocket))
		{
			continue;
		}
		sProcess.state = stateConnected;
		//processLock.lock_wait();
		//(*pActiveCount)--;
		//processLock.lock_release();
		--activeCount;
		do
		{
			HttpConnection connectionDown(socket);
			const char * host = "localhost";
			int port = 8888;
			Socket upSocket;
			if(!upSocket.connectTo(host, (ushort)port))
			{
				log.error("connectTo error %d", upSocket.socket);
				return false;
			}
			else
			{
				HttpConnection up(upSocket);
				log.trace("connection to <%s>:%d established %d", host, port, upSocket.socket);
				connectionDown.forward(up,config);
			}
		} while (false);
		sProcess.countConnections++;
		if (config.maxConnectionsPerProcess &&
			config.maxConnectionsPerProcess <= sProcess.countConnections)
		{
			log.warn("count of max connections reached %d", sProcess.countConnections);
			break;
		}
		if (!activeCount.conditionalIncremenent(config.countProcesses))
		{
			log.warn("count of max processes reached %d", config.countProcesses);
			break;
		}
	}
	log.trace( "exiting worker loop\n");
	return true;
}

bool LoupServer::loop()
{
	Func func(__PRETTY_FUNCTION__);

	while (!quit)
	{
		sleep((unsigned int)config.sleepTime);
		if (sighupReceived)
		{

			sighupReceived = false;
			for (size_t index = 0; index < config.countProcesses; ++index)
			{
				kill(pProcessList[index].pid, SIGABRT);
			}
			break;
		}
	}
	return true;
}
void LoupServer::sighupHandler(int sig)
{
	Func func(__PRETTY_FUNCTION__);
	sighupReceived = true;
}
bool LoupServer::SigHandlers::setHandlers()
{
	Func func(__PRETTY_FUNCTION__);
	setHandler(SIGCHLD, SIG_DFL);
	setHandler(SIGTERM, SIG_DFL);
	setHandler(SIGABRT, sighupHandler);
	setHandler(SIGHUP, sighupHandler);
	return true;
}
LoupServer::SigHandlers::SignalFunc *LoupServer::SigHandlers::setHandler(int signumber, SignalFunc * function)
{
	Func func(__PRETTY_FUNCTION__);
	struct sigaction action, oact;

	action.sa_handler = function;
	sigemptyset(&action.sa_mask);
	action.sa_flags = 0;
	if (signumber == SIGALRM) {
#ifdef SA_INTERRUPT
		action.sa_flags |= SA_INTERRUPT;
#endif
	}
	else {
#ifdef SA_RESTART
		action.sa_flags |= SA_RESTART;
#endif
	}

	if (sigaction(signumber, &action, &oact) < 0)
		return SIG_ERR;

	return oact.sa_handler;
}
