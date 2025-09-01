#pragma once

#include "Engine/Component.h"
#include "Engine/Math/Math.h"

class PhysicsComponent : public Component
{
public:

	float getMass() const { return mass; }
	const V4& getVelocity() const { return velocity; }
	void setVelocity(const V4& v) { velocity = v; }

protected:

	float mass = 1.0f;
	V4 velocity = V4::zero();
};