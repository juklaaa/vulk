#pragma once

#include "Engine/Core/Class.h"
#include "Engine/Core/Object.h"

class TestObject : public Object
{
	DECLARE_CLASS(TestObject)
	
public:
	
	bool myBool = true;
	int myInt = 100;
	float myFloat = -50.0f;
	
};

void testTestObject();