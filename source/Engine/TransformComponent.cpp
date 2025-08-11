#include "TransformComponent.h"
#include "Scene.h"

void TransformComponent::updateWorldTransform()
{
	worldTransform = transform;
	Scene* scene = owner->getScene();
	while (scene)
	{
		worldTransform = scene->getTransformComponent().getTransform() * worldTransform;
		scene = scene->getScene();
	}
}

const Mtx& TransformComponent::getWorldTransform() const
{
	//if (dirty)
	{
		const_cast<TransformComponent*>(this)->updateWorldTransform();
		dirty = false;
	}

	return worldTransform;
}

const Mtx& TransformComponent::getTransform() const
{ 
	return transform; 
}

Mtx& TransformComponent::getTransform()
{ 
	dirty = true; 
	return transform; 
}

void TransformComponent::setTransform(const Mtx& transform)
{ 
	this->transform = transform; 
	dirty = true; 
}
