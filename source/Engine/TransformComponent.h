#pragma once

#include "Component.h"
#include "Math/Math.h"

class TransformComponent : public Component
{
public:

	Mtx& accessTransform() { return transform; }
	const Mtx& getTransform() const { return transform; }
	void setTransform(const Mtx& transform) { this->transform = transform; }

private:

	Mtx transform = Mtx::identity();
};