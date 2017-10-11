//  main.cpp
// (C) Copyright 2017 by Martin Brunn  see License.txt for details
//
#include <stdlib.h>
#include "LoupServer.h"

Logger log;
Logger accessLog;
int main(int argc, char ** argv)
{
	LoupServer server;

	if (server.initServer(argc, argv))
		server.runServer();
	else
		exit(9);
	exit(0);
}