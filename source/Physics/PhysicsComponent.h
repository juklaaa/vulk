#pragma once

#include "Engine/Component.h"
#include "Engine/Math/Math.h"

class PhysicsComponent : public Component
{
public:

	enum Flags : uint8_t
	{
		None,
		Dynamic = 1,
		Gravity = 1 << 1,
		Heavy = 1 << 2
	};

	const V4& getVelocity() const { return velocity; }
	PhysicsComponent* setVelocity(const V4& v) { velocity = v; return this; }
	const V4& getAngularVelocity() const { return angularVelocity; }
	PhysicsComponent* setAngularVelocity(const V4& v) { angularVelocity = v; return this; }
	const Mtx& getInertia() const { return intertia; }
	PhysicsComponent* setInertia(const Mtx& m) { intertia = m; return this; }
	float getMass() const { return mass; }
	PhysicsComponent* setMass(float m) { mass = m; return this; }
	float getRestitution() const { return restitution; }
	PhysicsComponent* setRestitution(float e) { restitution = e; return this; }

	Flags getFlags() const { return flags; }
	PhysicsComponent* setFlags(int f) { flags = (Flags)f; return this; }
	
protected:

	Mtx intertia = Mtx::identity();
	V4 angularVelocity = V4::zero(); // As axis-angle
	V4 velocity = V4::zero();
	float mass = 1.0f;
	float restitution = 1.0f;
	Flags flags = None;
};