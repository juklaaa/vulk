#pragma once
#include <unordered_map>
#include <string>

#include "Object.h"

#define DECLARE_CLASS(ClassName) \
	friend SpecificClass<ClassName>; \
	static SpecificClass<ClassName> class_; \
	static void RegisterFields();

#define DEFINE_CLASS(ClassName) \
	SpecificClass<ClassName> ClassName::class_(#ClassName); \
	void ClassName::RegisterFields()

struct Field
{
	virtual ~Field() = default; 
	
	std::string name;
	size_t offset = 0;
	
	virtual std::string toString(const Object* object) const = 0;
	virtual void fromString(Object* object, std::string_view value) const = 0;
};

template<typename T>
struct SpecificField : Field
{
	virtual std::string toString(const Object* object) const
	{
		auto asT = reinterpret_cast<const T*>(reinterpret_cast<const char*>(object) + offset); 
		return std::to_string(*asT);
	}
	
	virtual void fromString(Object* object, std::string_view value) const
	{
		// TODO:
	}
};

#define FIELD(Type, ClassName, FieldName) \
	auto field##FieldName = new SpecificField<Type>; \
	field##FieldName->name = #FieldName; \
	field##FieldName->offset = offsetof(ClassName, FieldName); \
	class_.fields.push_back(field##FieldName)

struct Class
{
	Class(std::string_view classNameIn);
	
	virtual ~Class() = default;
	virtual Object* newObject() const = 0;
	
	std::string className;
	
	std::vector<Field*> fields;
};

struct ClassRegistry
{
	static ClassRegistry& singleton();
	
	std::unordered_map<std::string, Class*> classes;
};

template<typename T>
struct SpecificClass : Class
{
	SpecificClass(std::string_view classNameIn)
		: Class(classNameIn)
	{
		T::RegisterFields();
	}
	
	virtual Object* newObject() const override
	{
		auto newT = new T;
		newT->thisClass = this;
		return newT;
	}
};
