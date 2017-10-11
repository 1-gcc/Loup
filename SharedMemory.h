//  SharedMemory.h
// (C) Copyright 2017 by Martin Brunn  see License.txt for details
//
#pragma once
#ifndef SHAREDMEMORY_H
#define SHAREDMEMORY_H 1

class SharedMemory
{
public:
	SharedMemory();
	~SharedMemory();
	void * create(size_t size);
protected:
	int file;
};

#endif