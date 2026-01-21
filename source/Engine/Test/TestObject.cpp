#include "TestObject.h"

#include <iostream>

#include "Engine/Log.h"

DEFINE_CLASS(TestObject)
{
	FIELD(bool, TestObject, myBool);
	FIELD(int, TestObject, myInt);
	FIELD(float, TestObject, myFloat);
}

class MyTestObject2 : public TestObject
{
	DECLARE_CLASS(MyTestObject2);
	
public:
	
	std::string str = "Hello";
};

DEFINE_CLASS(MyTestObject2)
{
	
}

void testTestObject()
{
	Object* newObject = ClassRegistry::singleton().classes["TestObject"]->newObject();
	Object* newObject2 = ClassRegistry::singleton().classes["MyTestObject2"]->newObject();
	
	if (newObject2->getClass() == newObject->getClass())
	{
		
	}
	
	for (auto field : newObject->getClass()->fields)
	{
		std::cout << field->name << " = " << field->toString(newObject) << std::endl;
		//log(x, Verbose, "{} = {}", field->name, field->toString(newObject));
	}
	
	delete newObject;
	delete newObject2;
}