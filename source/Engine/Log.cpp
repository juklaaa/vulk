#include "Log.h"
#include "Common.h"
#include "Console/Console.h"

Logger& Logger::getSingleton()
{
	static Logger logger;
	return logger;
}

void Logger::init(Console* consoleIn)
{
	console = consoleIn;
	file.open("log.txt");
}

void Logger::writeString(LogSeverity severity, std::string_view str)
{
	if (console)
		console->print(str);
	file << str << std::endl;
}