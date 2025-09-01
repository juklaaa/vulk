#pragma once

#include "Component.h"
#include "Math/Math.h"

class TransformComponent : public Component
{
public:

	const Mtx& getTransform() const;
	void setTransform(const Mtx& transform);
	const Mtx& getWorldTransform() const;

private:

	void updateWorldTransform();

	Mtx transform = Mtx::indentity();
	mutable Mtx worldTransform = Mtx::indentity();
	mutable bool dirty = true;
};