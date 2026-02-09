#include "Class.h"

Class::Class(std::string_view classNameIn, const Class* baseClass)
	: base(baseClass)
	, className(classNameIn)
{
	ClassRegistry::singleton().classes[std::string(classNameIn)] = this;
}

std::vector<Field*> Class::getAllFields() const
{
	std::vector<Field*> result;
	if (base)
		result = base->getAllFields();
	
	result.insert(result.end(), fields.begin(), fields.end());
	return result;
}

ClassRegistry& ClassRegistry::singleton()
{
	static ClassRegistry instance;
	return instance;
}
