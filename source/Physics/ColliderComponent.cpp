#include "ColliderComponent.h"
#include "Engine/Actor.h"
#include "Engine/TransformComponent.h"
#include "Engine/Log.h"

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

void ColliderComponent::setLocalTransform(const Mtx& transform_)
{
	transform = transform_;
}

const Mtx& ColliderComponent::getLocalTransform() const
{
	return transform;
}

Mtx ColliderComponent::getTransform() const
{
	return transform * owner->getTransformComponent().getTransform();
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
	bool swappedColliders = false;
	if (colliderA->getType() > colliderB->getType())
	{
		std::swap(colliderA, colliderB);
		swappedColliders = true;
	}

	auto mediatorIt = mediators.find({ colliderA->getType(), colliderB->getType() });
	assert(mediatorIt != mediators.end());
	CollisionMediator* mediator = mediatorIt->second;
	auto collision = mediator->intersects(*colliderA, *colliderB, std::move(context));
	if (collision && swappedColliders)
		collision->normal *= -1.0f;
	return collision;
}

std::optional<Collision> SphereSphereCollisionMediator::intersects(const ColliderComponent& collider1, const ColliderComponent& collider2, std::optional<ColliderComponent::Context> context) const
{
	Mtx cwt1 = collider1.getTransform();
	Mtx cwt2 = collider2.getTransform();

	float r1 = 0.5f * V4(cwt1[0][0], cwt1[1][0], cwt1[2][0], 0.0f).length();
	float r2 = 0.5f * V4(cwt2[0][0], cwt2[1][0], cwt2[2][0], 0.0f).length();
	float r = r1 + r2;

	V4 c1 = cwt1.getPosition();
	V4 c2 = cwt2.getPosition();
	V4 dist = c2 - c1;
	if (dist.length() < r)
	{
		V4 n = dist.normalize();
		V4 p = c1 + n * r1;
		return Collision{ p, n };
	}
	return{};
}

std::optional<Collision> SphereBoxCollisionMediator::intersects(const ColliderComponent& collider1, const ColliderComponent& collider2, std::optional<ColliderComponent::Context> context) const
{
	Mtx sphereT = collider1.getTransform();
	const BoxColliderComponent& box = static_cast<const BoxColliderComponent&>(collider2);
	Mtx boxT = box.getTransform();
	Mtx invBoxT = boxT.inversedTransform();
	Mtx sphereT_Box = sphereT * invBoxT;
	
	AABB aabb
	{
		V4{-0.5f, -0.5f, -0.5f, 1.0f},
		V4{0.5f, 0.5f, 0.5f, 1.0f}
	};
	V4 sphereC_Box = sphereT_Box.getPosition();
	V4 sphereCProj_Box = clamp(sphereC_Box, aabb.min, aabb.max);
	V4 d = sphereCProj_Box - sphereC_Box;
	float r = 0.5f * V4(sphereT[0][0], sphereT[1][0], sphereT[2][0], 0.0f).length();
	if (d.length() < r) 
	{
		V4 pos_World = sphereCProj_Box * boxT;
		V4 n = (pos_World - sphereT.getPosition()).normalize();
		return Collision{ pos_World, n };
	}

	return {};
}

#define TEST_SPHERE_VS_BOX \
collision = GeneralCollisionMediator::getSingleton().intersects(sphereC, boxC, {}); \
if (collision) \
{ \
	logLine(x, Verbose, "{} {}", collision->point, collision->normal); \
} \
else \
{ \
	logLine(x, Verbose, "no collision"); \
}
	
void testSphereBoxCollisions()
{
	Actor sphereActor;
	auto& sphereC = *sphereActor.addComponent<SphereColliderComponent>();
	Actor boxActor;
	auto& boxC = *boxActor.addComponent<BoxColliderComponent>();
	Mtx& sphereT = sphereActor.getTransformComponent().accessTransform();
	Mtx& boxT = boxActor.getTransformComponent().accessTransform();
	
	sphereT = Mtx::translate({-0.95f, 0.0f, 0.0f});
	std::optional<Collision> collision;
	if (false)
	{
		TEST_SPHERE_VS_BOX
		boxT = Mtx::translate({1.0, 0.0f, 0.0f});
		sphereT = Mtx::translate({1.95f, 0.0f, 0.0f});
		TEST_SPHERE_VS_BOX
		boxT = Mtx::scale({3.0, 1.0f, 1.0f});
		TEST_SPHERE_VS_BOX
	}
	boxT = (Mtx::scale({1.0, 1.0f, 1.0f}) * Mtx::rotate({0.0f, 0.0f, -PI/4})) * Mtx::translate({4.0f, 0.0f, 0.0f});
	sphereT = Mtx::translate({4.0f + 0.3f + 0.5f, 0.3f + 0.5f, 0.0f});
	TEST_SPHERE_VS_BOX
}

std::optional<Collision> SpherePlaneCollisionMediator::intersects(const ColliderComponent& collider1, const ColliderComponent& collider2, std::optional<ColliderComponent::Context> context) const
{
	Mtx cwt = collider1.getTransform();

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
