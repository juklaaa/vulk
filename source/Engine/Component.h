#pragma once

class Actor;

class Component
{
public:

	virtual ~Component() = default;

	void setActor(Actor* actor);
	Actor* getActor() { return owner; }
	const Actor* getActor() const { return owner; }

	virtual void tick(float dt) {}

protected:

	Actor* owner = nullptr;
};