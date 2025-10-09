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
	virtual std::optional<V4> intersects(const ColliderComponent& collider1, const ColliderComponent& collider2) const = 0;
};

class GeneralCollisionMediator : public CollisionMediator
{
public:

	GeneralCollisionMediator();
	~GeneralCollisionMediator();
	virtual std::optional<V4> intersects(const ColliderComponent& collider1, const ColliderComponent& collider2) const override;

	static GeneralCollisionMediator& getSingleton();

private:

	std::unordered_map<std::pair<ColliderComponent::Type, ColliderComponent::Type>, CollisionMediator*> mediators;
};

std::optional<V4> ColliderComponent::intersects(ColliderComponent& other) const
{
	return GeneralCollisionMediator::getSingleton().intersects(*this, other);
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
	virtual std::optional<V4> intersects(const ColliderComponent& collider1, const ColliderComponent& collider2) const override;
};

class SphereBoxCollisionMediator : public CollisionMediator
{
	virtual std::optional<V4> intersects(const ColliderComponent& collider1, const ColliderComponent& collider2) const override;
};

class SpherePlaneCollisionMediator : public CollisionMediator
{
	virtual std::optional<V4> intersects(const ColliderComponent& collider1, const ColliderComponent& collider2) const override;
};

class BoxBoxCollisionMediator : public CollisionMediator
{
	virtual std::optional<V4> intersects(const ColliderComponent& collider1, const ColliderComponent& collider2) const override;
};

class PlanePlaneCollisionMediator : public CollisionMediator
{
	virtual std::optional<V4> intersects(const ColliderComponent& collider1, const ColliderComponent& collider2) const override
	{
		return {};
	}
};

GeneralCollisionMediator::GeneralCollisionMediator()
{
	mediators[{ColliderComponent::Type::Sphere, ColliderComponent::Type::Sphere}] = new SphereSphereCollisionMediator;
	mediators[{ColliderComponent::Type::Sphere, ColliderComponent::Type::Box}] = new SphereSphereCollisionMediator;
	mediators[{ColliderComponent::Type::Sphere, ColliderComponent::Type::Plane}] = new SpherePlaneCollisionMediator;
	mediators[{ColliderComponent::Type::Box, ColliderComponent::Type::Box}] = new BoxBoxCollisionMediator;
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

std::optional<V4> GeneralCollisionMediator::intersects(const ColliderComponent& collider1, const ColliderComponent& collider2) const
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
	return mediator->intersects(*colliderA, *colliderB);
}

std::optional<V4> SphereSphereCollisionMediator::intersects(const ColliderComponent& collider1, const ColliderComponent& collider2) const
{
	Mtx cwt1 = collider1.getWorldTransform();
	Mtx cwt2 = collider2.getWorldTransform();

	float r1 = 0.5f * V4(cwt1[0][0], cwt1[1][0], cwt1[2][0], 0.0f).length();
	float r2 = 0.5f * V4(cwt2[0][0], cwt2[1][0], cwt2[2][0], 0.0f).length();

	V4 centerLine = cwt2.getPosition() - cwt1.getPosition();

	if (centerLine.length() < r1 + r2)
	{
		return centerLine.normalize();
	}

	return {};
}

std::optional<V4> SphereBoxCollisionMediator::intersects(const ColliderComponent& collider1, const ColliderComponent& collider2) const
{
	Mtx sphereT = collider1.getWorldTransform();
	float r = 0.5f * V4(sphereT[0][0], sphereT[1][0], sphereT[2][0], 0.0f).length();
	V4 sphereC = sphereT.getPosition();
	const BoxColliderComponent& box = static_cast<const BoxColliderComponent&>(collider2);
	Mtx invBoxT = box.getWorldTransform().inversedTransform();

	// TODO: move sphere into invBoxT and calculate the collision

	return {};
}

std::optional<V4> SpherePlaneCollisionMediator::intersects(const ColliderComponent& collider1, const ColliderComponent& collider2) const
{
	Mtx cwt = collider1.getWorldTransform();

	float r = 0.5f * V4(cwt[0][0], cwt[1][0], cwt[2][0], 0.0f).length();

	V4 planeEq = static_cast<const PlaneColliderComponent&>(collider2).getEquation();
	float A = planeEq.x, B = planeEq.y, C = planeEq.z, D = planeEq.w;
	float x0 = cwt[3][0], y0 = cwt[3][1], z0 = cwt[3][2];
	float d = fabs(A*x0 + B*y0 + C*z0 + D)/ sqrt(A*A + B*B + C*C);
	
	if (d < r)
	{
		V4 normal = V4{ A, B, C, 0.0f }.normalize();
		return normal;
	}
		
	return {};
}

std::optional<V4> BoxBoxCollisionMediator::intersects(const ColliderComponent& collider1, const ColliderComponent& collider2) const
{
	// TODO
	return {};
}