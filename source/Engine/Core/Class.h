#pragma once
#include <cassert>
#include <memory>
#include <sstream>
#include <unordered_map>
#include <string>

#include "Object.h"

#define DECLARE_CLASS(ClassName) \
	friend SpecificClass<ClassName>; \
	static SpecificClass<ClassName> class_; \
	static void RegisterFields(); \
	virtual const Class* getClass() const override { return &class_; } \
	public: \
	static const Class* staticGetClass() { return &class_; } \
	private:

#define DEFINE_CLASS(ClassName, BaseClass) \
	SpecificClass<ClassName> ClassName::class_(#ClassName, BaseClass::staticGetClass()); \
	void ClassName::RegisterFields() \
	{ \
		using ThisClass = ClassName; \

struct Field
{
	virtual ~Field() = default; 
	
	std::string name;
	size_t offset = 0;
	const Field* parent = nullptr;
	
	virtual std::string toString(const Object* object) const { return {}; }
	virtual void fromString(Object* object, std::string_view value) const {}
	
	template <typename T>
	T& interpret(Object* object) const
	{
		if (parent)
		{
			assert(parent->isVector());
			if constexpr (!std::is_same_v<T, bool>)
			{
				auto asVec = reinterpret_cast<std::vector<T>*>(reinterpret_cast<char*>(object) + parent->offset); 
				return (*asVec)[offset];
			}
			else
			{
				static_assert("Don't use vector<bool> in reflexion, user vector<char> instead!");
				// ...because vector<bool> is fucked up
			}
		}
		
		auto asT = reinterpret_cast<T*>(reinterpret_cast<char*>(object) + offset); 
		return *asT;
	}
	
	template <typename T>
	const T& interpret(const Object* object) const
	{
		if (parent)
		{
			assert(parent->isVector());
			if constexpr (!std::is_same_v<T, bool>)
			{
				auto asVec = reinterpret_cast<const std::vector<T>*>(reinterpret_cast<const char*>(object) + parent->offset); 
				return (*asVec)[offset];
			}
			else
			{
				static_assert("Don't use vector<bool> in reflexion, user vector<char> instead!");
				// ...because vector<bool> is fucked up
			}
		}
		
		auto asT = reinterpret_cast<const T*>(reinterpret_cast<const char*>(object) + offset); 
		return *asT;
	}
	
	virtual bool isObjectPtr() const { return false; }
	virtual bool isVector() const { return false; }
	virtual std::vector<std::unique_ptr<Field>> getSubfields(const Object* object) const { return {}; }
	virtual std::vector<std::unique_ptr<Field>> createSubfields(Object* object, int num) const { return {}; }
};

template<typename T>
struct SpecificField : Field
{
	virtual std::string toString(const Object* object) const
	{
		std::ostringstream ss;
		ss << interpret<T>(object);
		return ss.str();
	}
	
	virtual void fromString(Object* object, std::string_view value) const
	{
		std::istringstream ss{std::string(value)};
		ss >> interpret<T>(object);
	}
};

template<>
struct SpecificField<std::string> : Field
{
	virtual std::string toString(const Object* object) const
	{
		return interpret<std::string>(object);
	}
	
	virtual void fromString(Object* object, std::string_view value) const
	{
		interpret<std::string>(object) = std::string(value);
	}
};

template<typename T>
concept ObjectPtr = std::is_convertible<T, Object*>::value;

template<ObjectPtr T> 
struct SpecificField<T> : Field
{
	virtual bool isObjectPtr() const override { return true; }
};

template<typename VecT>
struct SpecificField<std::vector<VecT>>: Field
{
	virtual bool isVector() const override { return true; }
	
	std::vector<std::unique_ptr<Field>> getSubfields(const Object* object) const override
	{
		auto& asVec = interpret<std::vector<VecT>>(object);
		std::vector<std::unique_ptr<Field>> subfields;
		for (int i = 0; i < asVec.size(); ++i)
		{
			auto subField = std::make_unique<SpecificField<VecT>>();
			subField->parent = this;
			subField->offset = i;
			subfields.push_back(std::move(subField));
		}
		return subfields;
	}
	
	std::vector<std::unique_ptr<Field>> createSubfields(Object* object, int num) const override
	{
		auto& asVec = interpret<std::vector<VecT>>(object);
		asVec.resize(num);
		return getSubfields(object);
	}
};

#define FIELD(Type, FieldName) \
	auto field##FieldName = new SpecificField<Type>; \
	field##FieldName->name = #FieldName; \
	field##FieldName->offset = offsetof(ThisClass, FieldName); \
	class_.fields.push_back(field##FieldName)

struct Class
{
	Class(std::string_view classNameIn, const Class* baseClass);
	
	virtual ~Class() = default;
	virtual Object* newObject() const = 0;
	
	const Class* base = nullptr;
	std::string className;
	
	std::vector<Field*> fields;
	std::vector<Field*> getAllFields() const;
};

struct ClassRegistry
{
	static ClassRegistry& singleton();
	
	Class* find(std::string_view classNameIn) { auto it = classes.find(std::string(classNameIn)); return it != classes.end() ? it->second : nullptr; }
	
	std::unordered_map<std::string, Class*> classes;
};

template<typename T>
struct SpecificClass : Class
{
	SpecificClass(std::string_view classNameIn, const Class* baseClass)
		: Class(classNameIn, baseClass)
	{
		T::RegisterFields();
	}
	
	virtual Object* newObject() const override
	{
		auto newT = new T;
		return newT;
	}
};
