#pragma once
#include "Common.h"

struct GlobalVarBase;

struct GlobalVarRegistry 
{
	static GlobalVarRegistry& getSingleton();
	
	std::unordered_map<std::string, GlobalVarBase*> variables;
};

struct GlobalVarBase
{
	GlobalVarBase(std::string_view name);
	virtual ~GlobalVarBase() = default;
	
	virtual void fromString(std::string_view str) = 0;
	virtual std::string toString() const = 0;
};

template <typename T>
struct GlobalVar : GlobalVarBase
{
	GlobalVar(std::string_view name, T initialValue)
		: GlobalVarBase(name)
		, value(initialValue)
	{
	}
	
	T get() { return value; }
	void set(T newValue) { value = newValue; }
	
	T value;
	
	virtual void fromString(std::string_view str) override
	{
		std::string string(str);
		std::stringstream ss(string);
		ss >> value;
	}
	
	virtual std::string toString() const override
	{
		return std::to_string(value);
	}
};