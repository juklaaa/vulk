#include "Component.h"
#include "Actor.h"

DEFINE_CLASS(Component, Object)
	FIELD(Actor*, owner);
}

void Component::setActor(Actor* actor)
{
	owner = actor;
}