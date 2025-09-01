#include "TransformComponent.h"
#include "Scene.h"

void TransformComponent::updateWorldTransform()
{
	if (owner->getScene())
		worldTransform = owner->getScene()->getTransformComponent().getWorldTransform() * transform;
	else
		worldTransform = transform;

	dirty = false;
}

const Mtx& TransformComponent::getWorldTransform() const
{
	//if (dirty)
	{
		const_cast<TransformComponent*>(this)->updateWorldTransform();
	}

	return worldTransform;
}

const Mtx& TransformComponent::getTransform() const
{ 
	return transform; 
}

void TransformComponent::setTransform(const Mtx& transform)
{ 
	this->transform = transform;
	dirty = true; 
}