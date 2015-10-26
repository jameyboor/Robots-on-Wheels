#pragma once

#pragma warning(disable:4996)

#include <iostream>
#include <string>
#include <sstream>
#include <chrono>
#include <stdarg.h>

enum LogLevels
{
	LOG_LEVEL_INFO,
	LOG_LEVEL_WARNING,
	LOG_LEVEL_FATAL
};

class Logger
{
public:
	static void Log(LogLevels logLevel, std::string text, ...)
	{
		std::ostringstream ss;
		ss << "[" << getCurrentTime() << "] ";

		switch (logLevel)
		{
		case LOG_LEVEL_INFO:
			ss << "[INFO] : ";
			break;

		case LOG_LEVEL_WARNING:
			ss << "[WARNING] : ";
			break;

		case LOG_LEVEL_FATAL:
			ss << "[FATAL] : ";
			break;
		}
		va_list args;
		va_start(args, text);
		char* formatted = new char[1024];
		vsnprintf(formatted, 1024, text.c_str(), args);
		va_end(args);
		ss << formatted << std::endl;
		std::cout << ss.str();
		delete[] formatted;
	}
private:
	static const std::string getCurrentTime()
	{
		time_t     now = time(0);
		struct tm  tstruct;
		char       buf[80];
		tstruct = *localtime(&now);
		strftime(buf, sizeof(buf), "%d-%m %X", &tstruct);

		return buf;
	}
};

#ifndef Log
#define Log Logger::Log
#endif
