#pragma once

#include "Engine/Component.h"
#include "Engine/Math/Math.h"

class PhysicsComponent : public Component
{
public:

	const V4& getVelocity() const { return velocity; }
	void setVelocity(const V4& v) { velocity = v; }
	const Quat& getAngularVelocity() const { return angularVelocity; }
	void setAngularVelocity(const Quat& m) { angularVelocity = m; }
	const Mtx& getInertia() const { return intertia; }
	void setInertia(const Mtx& m) { intertia = m; }
	float getMass() const { return mass; }
	void setMass(float m) { mass = m; }
	float getRestitution() const { return restitution; }
	void setRestitution(float e) { restitution = e; }	

	void setDynamic(bool b) { dynamic = b; }
	bool isDynamic() const { return dynamic; }
	
protected:

	Mtx intertia = Mtx::indentity();
	Quat angularVelocity = Quat::indentity();
	V4 velocity = V4::zero();
	float mass = 1.0f;
	float restitution = 1.0f;
	bool dynamic = true;
};