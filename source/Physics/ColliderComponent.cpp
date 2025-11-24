#include "ColliderComponent.h"
#include "Engine/Actor.h"
#include "Engine/TransformComponent.h"

namespace std
{
	template<> struct hash<std::pair<ColliderComponent::Type, ColliderComponent::Type>>
	{
		size_t operator()(std::pair<ColliderComponent::Type, ColliderComponent::Type> types) const
		{
			return (uchar)types.first * (uchar)ColliderComponent::Type::_Size + (uchar)types.second;
		}
	};
}

class CollisionMediator
{
public:

	virtual ~CollisionMediator() = default;
	virtual std::optional<Collision> intersects(const ColliderComponent& collider1, const ColliderComponent& collider2, std::optional<ColliderComponent::Context> context) const = 0;
};

class GeneralCollisionMediator : public CollisionMediator
{
public:

	GeneralCollisionMediator();
	~GeneralCollisionMediator();
	virtual std::optional<Collision> intersects(const ColliderComponent& collider1, const ColliderComponent& collider2, std::optional<ColliderComponent::Context> context) const override;

	static GeneralCollisionMediator& getSingleton();

private:

	std::unordered_map<std::pair<ColliderComponent::Type, ColliderComponent::Type>, CollisionMediator*> mediators;
};

std::optional<Collision> ColliderComponent::intersects(ColliderComponent& other, std::optional<Context> context) const
{
	if (context)
		context->owner = this;
	return GeneralCollisionMediator::getSingleton().intersects(*this, other, std::move(context));
}

void ColliderComponent::setTransform(const Mtx& transform_)
{
	transform = transform_;
}

const Mtx& ColliderComponent::getTransform() const
{
	return transform;
}

Mtx ColliderComponent::getWorldTransform() const
{
	return owner->getTransformComponent().getWorldTransform() * transform;
}

class SphereSphereCollisionMediator : public CollisionMediator
{
	virtual std::optional<Collision> intersects(const ColliderComponent& collider1, const ColliderComponent& collider2, std::optional<ColliderComponent::Context> context) const override;
};

class SphereBoxCollisionMediator : public CollisionMediator
{
	virtual std::optional<Collision> intersects(const ColliderComponent& collider1, const ColliderComponent& collider2, std::optional<ColliderComponent::Context> context) const override;
};

class SpherePlaneCollisionMediator : public CollisionMediator
{
	virtual std::optional<Collision> intersects(const ColliderComponent& collider1, const ColliderComponent& collider2, std::optional<ColliderComponent::Context> context) const override;
};

class BoxBoxCollisionMediator : public CollisionMediator
{
	virtual std::optional<Collision> intersects(const ColliderComponent& collider1, const ColliderComponent& collider2, std::optional<ColliderComponent::Context> context) const override;
};

class BoxPlaneCollisionMediator : public CollisionMediator
{
	virtual std::optional<Collision> intersects(const ColliderComponent& collider1, const ColliderComponent& collider2, std::optional<ColliderComponent::Context> context) const override;
};

class PlanePlaneCollisionMediator : public CollisionMediator
{
	virtual std::optional<Collision> intersects(const ColliderComponent& collider1, const ColliderComponent& collider2, std::optional<ColliderComponent::Context> context) const override { return {}; }
};

GeneralCollisionMediator::GeneralCollisionMediator()
{
	mediators[{ColliderComponent::Type::Sphere, ColliderComponent::Type::Sphere}] = new SphereSphereCollisionMediator;
	mediators[{ColliderComponent::Type::Sphere, ColliderComponent::Type::Box}] = new SphereBoxCollisionMediator;
	mediators[{ColliderComponent::Type::Sphere, ColliderComponent::Type::Plane}] = new SpherePlaneCollisionMediator;
	mediators[{ColliderComponent::Type::Box, ColliderComponent::Type::Box}] = new BoxBoxCollisionMediator;
	mediators[{ColliderComponent::Type::Box, ColliderComponent::Type::Plane}] = new BoxPlaneCollisionMediator;
	mediators[{ColliderComponent::Type::Plane, ColliderComponent::Type::Plane}] = new PlanePlaneCollisionMediator;
}

GeneralCollisionMediator::~GeneralCollisionMediator()
{
	for (auto& [key, ptr] : mediators)
		delete ptr;
}

GeneralCollisionMediator& GeneralCollisionMediator::getSingleton()
{
	static GeneralCollisionMediator instance;
	return instance;
}

std::optional<Collision> GeneralCollisionMediator::intersects(const ColliderComponent& collider1, const ColliderComponent& collider2, std::optional<ColliderComponent::Context> context) const
{
	// sort pair
	const ColliderComponent* colliderA = &collider1;
	const ColliderComponent* colliderB = &collider2;
	if (colliderA->getType() > colliderB->getType())
	{
		std::swap(colliderA, colliderB);
	}

	auto mediatorIt = mediators.find({ colliderA->getType(), colliderB->getType() });
	assert(mediatorIt != mediators.end());
	CollisionMediator* mediator = mediatorIt->second;
	return mediator->intersects(*colliderA, *colliderB, std::move(context));
}

std::optional<Collision> SphereSphereCollisionMediator::intersects(const ColliderComponent& collider1, const ColliderComponent& collider2, std::optional<ColliderComponent::Context> context) const
{
	Mtx cwt1 = collider1.getWorldTransform();
	Mtx cwt2 = collider2.getWorldTransform();

	float r1 = 0.5f * V4(cwt1[0][0], cwt1[1][0], cwt1[2][0], 0.0f).length();
	float r2 = 0.5f * V4(cwt2[0][0], cwt2[1][0], cwt2[2][0], 0.0f).length();
	float r = r1 + r2;

	V4 c1 = cwt1.getPosition();
	V4 c2 = cwt2.getPosition();

	V4 diff = c1 - c2;
	
	if (!context)
	{
		if (diff.length() < r)
		{
			V4 n = diff.normalize();
			V4 p = c2 + n * r2;
			return Collision{ p, n };
		}
		return{};
	}

	V4 pc1 = context->prevPosition.getPosition();
	V4 v = c1 - pc1;
	V4 s = pc1 - c2;
	float a = v.dot(v);
	if (a < FLT_EPSILON) return {}; 
	float b = v.dot(s);
	if (b >= 0.0f) return {}; 
	float c = s.dot(s) - r * r;
	float d = b * b - a * c;
	if (d < 0.0f) return {}; 

	float t = (-b - sqrtf(d)) / a;
	if ( t > 1.0f) return {};


	V4 hitCenter = pc1 + v * t;
	V4 normal = (hitCenter - c2).normalize();
	V4 point = c2 + normal * r2;
	return Collision{ point, normal };
	
}

std::optional<Collision> SphereBoxCollisionMediator::intersects(const ColliderComponent& collider1, const ColliderComponent& collider2, std::optional<ColliderComponent::Context> context) const
{
	Mtx sphereT = collider1.getWorldTransform();
	V4 sphereC_World = sphereT.getPosition();
	float r = 0.5f * V4(sphereT[0][0], sphereT[1][0], sphereT[2][0], 0.0f).length();
	const BoxColliderComponent& box = static_cast<const BoxColliderComponent&>(collider2);
	Mtx boxT = box.getWorldTransform();
	Mtx boxR = boxT.getRotation();
	Mtx invBoxR = boxR.inversedTransform();
	Mtx boxAABB = invBoxR * boxT;
	Mtx sphereT_Box = invBoxR * sphereT;
	
	Mtx sphereTPrev_Box;
	if (context)
	{
		if (context->owner == &collider1)
		{
			sphereTPrev_Box = invBoxR * context->prevPosition;
		}
		else
		{
			sphereTPrev_Box = context->prevPosition.getRotation().inversedTransform() * sphereT;
		}
	}
	else
		return {};

	V4 sphereC_Box = sphereT_Box.getPosition();
	V4 spherePrevC_Box = sphereTPrev_Box.getPosition();
	V4 spherePath_Box = sphereC_Box - spherePrevC_Box;
	AABB aabb
	{
		V4{-0.5f, -0.5f, -0.5f, 1.0f} * boxAABB,
		V4{0.5f, 0.5f, 0.5f, 1.0f} * boxAABB
	};
	AABB aabbEx = aabb;
	aabbEx.min -= V4{ r, r, r, 0.0f };
	aabbEx.max += V4{ r, r, r, 0.0f };

	auto rayResult = intersectRayAABB(spherePrevC_Box, sphereC_Box - spherePrevC_Box, aabbEx);
	if (!rayResult || rayResult->t > 1.0f)
		return {};

	
	const float eps =  0.5f;
	
	V4 p = rayResult->point;
	V4 p2min = p - aabbEx.min;
	V4 p2max = p - aabbEx.max;
	
	V4 normal{ 0.0f, 0.0f, 1.0f};
	if (fabs(p2min.x) < eps)
		normal = V4{ -1.0f, 0.0f, 0.0f };
	if (fabs(p2max.x) < eps)
		normal = V4{ 1.0f, 0.0f, 0.0f };
	if (fabs(p2min.y) < eps)
		normal = V4{ 0.0f, -1.0f, 0.0f };
	if (fabs(p2max.y) < eps)
		normal = V4{ 0.0f, 1.0f, 0.0f };
	if (fabs(p2min.z) < eps)
		normal = V4{ 0.0f, 0.0f, -1.0f };
	if (fabs(p2max.z) < eps)
		normal = V4{ 0.0f, 0.0f, 1.0f };

	V4 point = rayResult->point * boxR;
	V4 n = normal * boxR;

	return Collision{ point, n.normalize()};
}

std::optional<Collision> SpherePlaneCollisionMediator::intersects(const ColliderComponent& collider1, const ColliderComponent& collider2, std::optional<ColliderComponent::Context> context) const
{
	Mtx cwt = collider1.getWorldTransform();

	float r = 0.5f * V4(cwt[0][0], cwt[1][0], cwt[2][0], 0.0f).length();

	V4 planeEq = static_cast<const PlaneColliderComponent&>(collider2).getEquation();
	float A = planeEq.x, B = planeEq.y, C = planeEq.z, D = planeEq.w;
	V4 sphereC = cwt.getPosition();
	float x0 = sphereC.x, y0 = sphereC.y, z0 = sphereC.z;
	float d = fabs(A*x0 + B*y0 + C*z0 + D)/ sqrt(A*A + B*B + C*C);
	
	if (d < r)
	{
		V4 normal = V4{ A, B, C, 0.0f }.normalize();
		return Collision{ sphereC - normal * r, normal };
	}
		
	return {};
}

std::optional<Collision> BoxBoxCollisionMediator::intersects(const ColliderComponent& collider1, const ColliderComponent& collider2, std::optional<ColliderComponent::Context> context) const
{
	// TODO
	return {};
}

std::optional<Collision> BoxPlaneCollisionMediator::intersects(const ColliderComponent& collider1, const ColliderComponent& collider2, std::optional<ColliderComponent::Context> context) const
{
	// TODO
	return {};
}

std::optional<RayIntersectResult> intersectRayAABB(const V4& point, const V4& dir, const AABB& aabb)
{
	float tmin = 0.0f;
	float tmax = std::numeric_limits<float>::max();
	for (int i = 0; i < 3; i++)
	{
		
		if (fabs(dir[i]) < FLT_EPSILON)
		{
			if (point[i] < aabb.min[i] || point[i] > aabb.max[i])
				return {};
		}
		else
		{
			float ood = 1.0f / dir[i];
			float t1 = (aabb.min[i] - point[i]) * ood;
			float t2 = (aabb.max[i] - point[i]) * ood;
			if (t1 > t2) 
				std::swap(t1, t2);
			if (t1 > tmin)
				tmin = t1;
			if (t2 < tmax)
				tmax = t2;
			if (tmin > tmax)
				return {};
		}
	}
	
	return RayIntersectResult{ point + dir * tmin, tmin };
}