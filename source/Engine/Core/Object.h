#pragma once

struct Class;

class Object
{
	friend struct Class;
	template <typename T>
	friend struct SpecificClass;
public:
	
	virtual ~Object() = default;
	
	virtual const Class* getClass() const { return nullptr; }
	static const Class* staticGetClass() { return nullptr; }
};
