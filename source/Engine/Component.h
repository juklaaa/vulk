#pragma once

class Actor;

class Component
{
public:

	virtual ~Component() = default;

	void setActor(Actor* actor);

	virtual void tick(float dt) {}

protected:

private:

	Actor* owner = nullptr;
};