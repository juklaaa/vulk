#include "TestObject.h"

#include "Common.h"
#include "Engine/Log.h"
#include "Engine/Core/SerializeObject.h"

class TestObject2;

class TestObject : public Object
{
	DECLARE_CLASS(TestObject)
	
public:
	
	bool myBool = true;
	int myInt = 100;
	float myFloat = -50.0f;
	TestObject* myTestObject = nullptr;
	std::string strBase = "Base";
};

DEFINE_CLASS(TestObject, Object)
	FIELD(bool, myBool);
	FIELD(int, myInt);
	FIELD(float, myFloat);
	FIELD(Object*, myTestObject);
	FIELD(std::string, strBase);
}

class TestObject2 : public TestObject
{
	DECLARE_CLASS(TestObject2)
	
public:
	
	std::string str = "Hello";
	std::vector<int> vecI;
	std::vector<std::string> vecS;
	std::vector<Object*> myObjects;
};

DEFINE_CLASS(TestObject2, TestObject)
	FIELD(std::string, str);
	FIELD(std::vector<int>, vecI);
	FIELD(std::vector<std::string>, vecS);
	FIELD(std::vector<Object*>, myObjects);
}

void testTestObject()
{
	TestObject testObject;
	testObject.myBool = false;
	testObject.myInt = 42;
	testObject.strBase = "TestObject";
	
	TestObject2 testObject2;
	testObject2.myFloat = 2137.0f;
	testObject2.str = "Hello World";
	testObject2.strBase = "TestObject2";
	testObject2.vecI = std::vector<int>{1, 2, 3};
	testObject2.vecS = std::vector<std::string>{"My", "vector", "of", "strings!"};
	TestObject testObject3;
	testObject3.strBase = "I'm a test object #3";
	testObject2.myObjects = std::vector<Object*>{ &testObject, &testObject2, &testObject3 };
	
	testObject.myTestObject = &testObject2;
	testObject2.myTestObject = &testObject;
	
	std::ifstream fin("test.json");
	if (!fin)
	{
		json j = serializeObject(&testObject);
		std::ofstream fout("test.json");
		fout << std::setw(4) << j;
	}
	else
	{
		json j;
		fin >> j;
		Object* obj = deserializeObject(j);
	}
}