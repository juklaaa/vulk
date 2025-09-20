#pragma once

#include "Engine/Component.h"
#include "Engine/Math/Math.h"

class PhysicsComponent : public Component
{
public:

	
	const V4& getVelocity() const { return velocity; }
	void setVelocity(const V4& v) { velocity = v; }
	float getMass() const { return mass; }
	void setMass(float m) { mass = m; }
	float getRestitution() const { return restitution; }
	void setRestitution(float e) { restitution = e; }	
	
protected:

	float mass = 1.0f;
	float restitution = 1.0f;
	V4 velocity = V4::zero();
};