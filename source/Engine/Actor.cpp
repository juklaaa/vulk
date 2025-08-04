#include "Actor.h"

Actor::~Actor()
{
	for (Component* component : components)
		delete component;
}

Component* Actor::addComponent(Component* component)
{
	component->setActor(this);
	components.push_back(component); 
	return component;
}

TransformComponent& Actor::getTransformComponent()
{
	return transformComponent;
}

void Actor::tick(float dt)
{
	for (Component* component : components)
		component->tick(dt);
}