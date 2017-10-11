//  Locker.h
// (C) Copyright 2017 by Martin Brunn  see License.txt for details
//
#pragma once

#include <fcntl.h>

class Locker
{
public:
	Locker();
	~Locker();
	bool init();
	bool lock_release();
	bool lock_wait();
protected:
	int file;
	struct flock lock;
	struct flock unlock;

};

