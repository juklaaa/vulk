#pragma once

struct Class;

class Object
{
	friend struct Class;
	template <typename T>
	friend struct SpecificClass;
	
	const Class* thisClass = nullptr;
	
public:
	
	virtual ~Object() = default;
	
	const Class* getClass() const { return thisClass; }
};
