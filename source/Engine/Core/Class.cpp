#include "Class.h"

Class::Class(std::string_view classNameIn)
	: className(classNameIn)
{
	ClassRegistry::singleton().classes[std::string(classNameIn)] = this;
}

ClassRegistry& ClassRegistry::singleton()
{
	static ClassRegistry instance;
	return instance;
}