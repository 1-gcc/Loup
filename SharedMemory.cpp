//  SharedMemory.cpp
// (C) Copyright 2017 by Martin Brunn  see License.txt for details
//
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include "SharedMemory.h"



SharedMemory::SharedMemory()
{
	file = -1;
}


SharedMemory::~SharedMemory()
{
	if (file >= 0)
	{
		close(file);
	}
}
void * SharedMemory::create(size_t size)
{
	char templateFName[] = "/tmp/Loup_SharedMemoryXXXXXXXX";
	file = mkstemp(templateFName);
	if (file == -1)
	{
		return NULL;
	}
	unlink(templateFName);
	if (ftruncate(file, size) == -1)
	{
		return NULL;
	}
	void * p = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, file, 0);
	if(p != NULL)
		memset(p, 0, size);
	return p;
}
