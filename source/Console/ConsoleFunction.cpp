#include "ConsoleFunction.h"

ConsoleFunction::ConsoleFunction(std::string_view name, std::string(*f)(std::vector<std::string>))
	: func(f)
{
	ConsoleFunctionRegistry::getSingleton().functions[std::string(name)] = this;
}

ConsoleFunctionRegistry& ConsoleFunctionRegistry::getSingleton()
{
	static ConsoleFunctionRegistry functionRegistry;
	return functionRegistry;
}
