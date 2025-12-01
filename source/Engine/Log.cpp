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

void Logger::writeString(std::string_view str)
{
	std::cout << str << std::endl;
	OutputDebugStringA(str.data());
	OutputDebugStringA("\n");
	file << str << std::endl;
}