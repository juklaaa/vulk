#pragma once

#include <fstream>
#include <format>
#include <string>

#include "TypesText.h"

enum LogSeverity { Error, Warning, Log, Verbose };

struct Logger
{
	static Logger& getSingleton();

	void init(class Console* console);
	void writeString(LogSeverity severity, std::string_view str);

	template <typename... Types>
	void writeLog(LogSeverity severity, const char* filename, int lineNumber, const std::format_string<Types...> fmt, Types&&... args)
	{
		std::string line = std::format("{}({},1): {}", filename, lineNumber, std::format(fmt, std::forward<Types>(args)...));
		writeString(severity, line);
	}

	std::ofstream file;
	Console* console;
};

#define logLine(cat, severity, ...) \
Logger::getSingleton().writeLog(severity, __FILE__, __LINE__, __VA_ARGS__);
