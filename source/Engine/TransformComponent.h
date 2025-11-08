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

	Mtx transform = Mtx::identity();
	mutable Mtx worldTransform = Mtx::identity();
	mutable bool dirty = true;
};