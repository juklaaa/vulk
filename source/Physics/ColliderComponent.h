#pragma once

#include "Common.h"
#include "Engine/Component.h"
#include "Engine/Math/Math.h"

class Actor;

class ColliderComponent : public Component
{
public:

	enum class Type : uchar
	{
		Sphere,
		Box,
		Plane,

		_Size
	};

	virtual Type getType() const = 0;

	bool intersects(ColliderComponent& other) const;
	void setTransform(const Mtx& transform);
	const Mtx& getTransform() const;
	Mtx getWorldTransform() const;

protected:
	Mtx transform = Mtx::indentity();
};

class SphereColliderComponent : public ColliderComponent
{
public:
	virtual Type getType() const override { return Type::Sphere; }
};

class BoxColliderComponent : public ColliderComponent
{
public:
	virtual Type getType() const override { return Type::Box; }
};

class PlaneColliderComponent : public ColliderComponent
{
public:
	virtual Type getType() const override { return Type::Plane; }

	V4 getEquation() const { return equation; }

protected:
	V4 equation;
};