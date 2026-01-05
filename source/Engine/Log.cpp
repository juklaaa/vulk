#include "Log.h"
#include "Common.h"
#include "Windows.h"

Logger& Logger::getSingleton()
{
	static Logger logger;
	return logger;
}

void Logger::init()
{
	file.open("log.txt");
}

void Logger::writeString(LogSeverity severity, std::string_view str)
{
	if (severity == LogSeverity::Error ||
		severity == LogSeverity::Warning)
	{
		std::cerr << str << std::endl;
	}
	else
		std::cout << str << std::endl;
	OutputDebugStringA(str.data());
	OutputDebugStringA("\n");
	file << str << std::endl;
}