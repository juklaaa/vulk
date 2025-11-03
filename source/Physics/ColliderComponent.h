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

	struct CollisionContext
	{
		Mtx prevPosition;
		const ColliderComponent* owner = nullptr;
	};

	std::optional<V4> intersects(ColliderComponent& other, std::optional<CollisionContext> context) const;
	void setTransform(const Mtx& transform);
	const Mtx& getTransform() const;
	Mtx getWorldTransform() const;

protected:
	Mtx transform = Mtx::identity();
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

	void setEquation(V4 e) { equation = e; }

protected:
	V4 equation = { 0.0f, 1.0f, 1.0f, 0.0f };	
};