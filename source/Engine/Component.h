#pragma once
#include "Core/Class.h"
#include "Core/Object.h"

class Actor;

class Component : public Object
{
	DECLARE_CLASS(Component)
	
public:

	virtual ~Component() = default;

	void setActor(Actor* actor);
	Actor* getActor() { return owner; }
	const Actor* getActor() const { return owner; }

	virtual void tick(float dt) {}

protected:

	Actor* owner = nullptr;
};