#include "GlobalVar.h"

GlobalVarRegistry& GlobalVarRegistry::getSingleton()
{
	static GlobalVarRegistry globalVarRegistry;
	return globalVarRegistry;
}

GlobalVarBase::GlobalVarBase(std::string_view name)
{
	GlobalVarRegistry::getSingleton().variables[std::string(name)] = this;
}
