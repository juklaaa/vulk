#pragma once
#include <string>
#include <unordered_map>
#include <vector>

struct ConsoleFunction
{
	ConsoleFunction(std::string_view name, std::string (*f)(std::vector<std::string>));
	std::string (*func)(std::vector<std::string>);
};

struct ConsoleFunctionRegistry
{
	static ConsoleFunctionRegistry& getSingleton();
	
	std::unordered_map<std::string, ConsoleFunction*> functions;
};