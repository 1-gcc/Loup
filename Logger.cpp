//  Logger.cpp
// (C) Copyright 2017 by Martin Brunn  see License.txt for details
//
#include <stdio.h>
#include <dlfcn.h>
#include <stdarg.h>
#include "Logger.h"
extern Logger log;
void (*Logger::xtrace)(const char*file,const char*s, const char*format, va_list&vlist) = NULL;
Logger::~Logger()
{
	if(lib)
		dlclose(lib);
	lib = NULL;
}
Func::Func(const char*name):name(name),trace(false)
{
	log.debug("%s entry", name);
}
Func::Func(bool trace,const char*name) : name(name),trace(trace)
{
	if (trace)
		log.trace("%s entry", name);
	else
		log.debug("%s entry", name);
}
Func::~Func()
{
	if (trace)
		log.trace("%s exit", name);
	else
		log.debug("%s exit", name);
}

Logger::Logger()
{
	flag = RTLD_NOW | RTLD_GLOBAL;
	libraryName = "logger.so";
	lib = NULL;
	psetTraceFile = NULL;
/*	trace = log;
	//trace = notrace;
	debug = notrace;
	info = notrace;
	warn = notrace;
	note = notrace;
	error = log;*/
	bTrace = true;
	bWarn = false;
	bError = false;
	bNote = false;
	bInfo = false;
	bDebug = false;


	tracefile = "TraceFile_20170930.trc";
}
bool Logger::load(const std::set<std::string>&flags)
{
	void * lib = dlopen(libraryName,flag);
	if(lib == NULL)
	{
		char* err = dlerror();
		fprintf(stderr,"dlopen error %s\n",err);
		return false;
	}
	psetTraceFile = (char* (*)(const char*)) dlsym(lib,"setTraceFile");
	if(psetTraceFile == NULL)
	{
		char * err = dlerror();
		fprintf(stderr,"dlsym error %s\n",err);
		return false;
	}
	xtrace = (void (*)(const char*,const char*,const char*,va_list&))dlsym(lib,"trace");
	if(xtrace == NULL)
	{
		char* err = dlerror();
		fprintf(stderr,"dlsym error %s\n",err);
		return false;
	}
	
	for (std::set<std::string>::iterator i = flags.begin(); i != flags.end(); ++i)
	{
		if (*i == "debug")
		{
			bDebug = true;
		}
		if (*i == "warn")
		{
			bWarn = true;
		}
		if (*i == "info")
		{
			bInfo = true;
		}
		if (*i == "note")
		{
			bNote = true;
		}
		if (*i == "error")
		{
			bError = true;
		}
	}
	psetTraceFile(tracefile.c_str());
	return true;

}
void Logger::setTraceFile(const char*file)
{
	tracefile = file;
}
void Logger::trace(const char*format, ...)
{
	if (!bTrace)
		return;
	va_list(vlist);
	va_start(vlist, format);

	xtrace(tracefile.c_str(), "TRC", format, vlist);
}
void Logger::warn(const char*format, ...)
{
	if (!bWarn)
		return;
	va_list(vlist);
	va_start(vlist, format);

	xtrace(tracefile.c_str(), "WRN", format, vlist);
}
void Logger::error(const char*format, ...)
{
	if (!bError)
		return;
	va_list(vlist);
	va_start(vlist, format);

	xtrace(tracefile.c_str(), "ERR", format, vlist);
}
void Logger::note(const char*format, ...)
{
	if (!bNote)
		return;
	va_list(vlist);
	va_start(vlist, format);

	xtrace(tracefile.c_str(), "NOT", format, vlist);
}
void Logger::info(const char*format, ...)
{
	if (!bInfo)
		return;
	va_list(vlist);
	va_start(vlist, format);

	xtrace(tracefile.c_str(), "INF", format, vlist);
}
void Logger::debug(const char*format, ...)
{
	if (!bDebug)
		return;
	va_list(vlist);
	va_start(vlist, format);

	xtrace(tracefile.c_str(), "DBG", format, vlist);
}

void Logger::log(const char*format, ...)
{
	va_list(vlist);
	va_start(vlist, format);

	vfprintf(stderr, format,vlist);
	fprintf(stderr, "\n");
}
