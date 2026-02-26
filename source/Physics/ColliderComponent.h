#pragma once

#include "Common.h"
#include "Engine/Component.h"
#include "Engine/Math/Math.h"

class Actor;

struct Collision
{
	V4 point;
	V4 normal;
};

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

	struct Context
	{
		Mtx prevPosition;
		const ColliderComponent* owner = nullptr;
	};

	std::optional<Collision> intersects(ColliderComponent& other, std::optional<Context> context) const;
	void setLocalTransform(const Mtx& transform);
	const Mtx& getLocalTransform() const;
	Mtx getTransform() const;

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

struct AABB
{
	V4 min = V4::zero();
	V4 max = V4::zero();
};

struct RayIntersectResult
{
	V4 point = V4::zero();
	float t = 0.0f;
};
std::optional<RayIntersectResult> intersectRayAABB(const V4& point, const V4& dir, const AABB& aabb);

void testSphereBoxCollisions();
