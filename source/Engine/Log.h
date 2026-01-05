#pragma once

#include <fstream>
#include <format>
#include <string>

enum LogSeverity { Error, Warning, Info, Verbose };

struct Logger
{
	static Logger& getSingleton();

	void init();
	void writeString(LogSeverity severity, std::string_view str);

	template <typename... Types>
	void writeLog(LogSeverity severity, const char* filename, int lineNumber, const std::format_string<Types...> fmt, Types&&... args)
	{
		std::string line = std::format("{}({},1): {}", filename, lineNumber, std::format(fmt, args...));
		writeString(severity, line);
	}

	std::ofstream file;
};

#define log(cat, severity, ...) \
Logger::getSingleton().writeLog(severity, __FILE__, __LINE__, __VA_ARGS__);