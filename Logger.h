//  Logger.h
// (C) Copyright 2017 by Martin Brunn  see License.txt for details
//
#define LOGGER_H

#include <set>
#include <string>
#include <stdarg.h>

class Logger
{
protected:
	int flag ;
	const char * libraryName ;
	std::string tracefile;
	void * lib ;
	void notrace(const char * format,...)
	{
	}
	bool bTrace;
	bool bWarn;
	bool bError;
	bool bNote;
	bool bInfo;
	bool bDebug;
	char * (*psetTraceFile)(const char*file);
public:
	Logger();
	virtual ~Logger();
	bool load(const std::set<std::string>&flags);
	static void (*xtrace)(const char*file,const char*s, const char*format, va_list&vlist);
	void trace(const char*format,...) ;
	void warn(const char*format, ...);
	void error(const char*format, ...);
	void note(const char*format, ...);
	void info(const char*format, ...);
	void debug(const char*format, ...);
	void log(const char*format, ...);
	void setTraceFile(const char*file);
};
class Func
{
public:
	Func(const char*name);
	Func(bool trace, const char*name);
	~Func();
	const char * name;
	bool trace;
};
