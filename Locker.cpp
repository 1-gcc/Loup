//  Locker.cpp
// (C) Copyright 2017 by Martin Brunn  see License.txt for details
//
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

#include "Logger.h"

#include "Locker.h"

extern Logger log;
Locker::Locker()
{
	lock.l_type = F_WRLCK;
	lock.l_whence = SEEK_SET;
	lock.l_start = 0;
	lock.l_len = 0;

	unlock.l_type = F_UNLCK;
	unlock.l_whence = SEEK_SET;
	unlock.l_start = 0;
	unlock.l_len = 0;

}
Locker::~Locker()
{
	if (file != -1)
	{
		close(file);
	}
}

bool Locker::init()
{
	Func func(__PRETTY_FUNCTION__);
	char templateFName[] = "/tmp/Loup_LockXXXXXXXX";
	file = mkstemp(templateFName);
	if (file == -1)
	{
		return false;
	}
	unlink(templateFName);

	return true;
}

bool Locker::lock_release()
{
	log.note("lock_release");
	while (fcntl(file, F_SETLKW64, &unlock) < 0)
	{
		log.error("fcntl error %d", errno);
		if (errno == EINTR)
		{
			continue;
		}
		return false;
	}
	return true;
}
bool Locker::lock_wait()
{
	Func func(__PRETTY_FUNCTION__);
	while (fcntl(file, F_SETLKW64, &lock) < 0)
	{
		log.error("fcntl error %d", errno);
		if (errno == EINTR)
		{
			continue;
		}
		return false;
	}
	return true;
}

