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
	virtual V4 intersects(const ColliderComponent& collider1, const ColliderComponent& collider2) const = 0;
};

class GeneralCollisionMediator : public CollisionMediator
{
public:

	GeneralCollisionMediator();
	~GeneralCollisionMediator();
	virtual V4 intersects(const ColliderComponent& collider1, const ColliderComponent& collider2) const override;

	static GeneralCollisionMediator& getSingleton();

private:

	std::unordered_map<std::pair<ColliderComponent::Type, ColliderComponent::Type>, CollisionMediator*> mediators;
};

V4 ColliderComponent::intersects(ColliderComponent& other) const
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
	virtual V4 intersects(const ColliderComponent& collider1, const ColliderComponent& collider2) const override;
};

class SphereBoxCollisionMediator : public CollisionMediator
{
	virtual V4 intersects(const ColliderComponent& collider1, const ColliderComponent& collider2) const override;
};

class SpherePlaneCollisionMediator : public CollisionMediator
{
	virtual V4 intersects(const ColliderComponent& collider1, const ColliderComponent& collider2) const override;
};

class BoxBoxCollisionMediator : public CollisionMediator
{
	virtual V4 intersects(const ColliderComponent& collider1, const ColliderComponent& collider2) const override;
};

class PlanePlaneCollisionMediator : public CollisionMediator
{
	virtual V4 intersects(const ColliderComponent& collider1, const ColliderComponent& collider2) const override
	{
		return  V4(0.0f, 0.0f, 0.0f, 0.0f);
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

V4 GeneralCollisionMediator::intersects(const ColliderComponent& collider1, const ColliderComponent& collider2) const
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

V4 SphereSphereCollisionMediator::intersects(const ColliderComponent& collider1, const ColliderComponent& collider2) const
{
	Mtx cwt1 = collider1.getWorldTransform();
	Mtx cwt2 = collider2.getWorldTransform();

	float r1 = 0.5f *V4(cwt1[0][0], cwt1[1][0], cwt1[2][0], 0.0f).length();
	float r2 = 0.5f * V4(cwt2[0][0], cwt2[1][0], cwt2[2][0], 0.0f).length();

	if (cwt1.getPosition().dist(cwt2.getPosition()) < r1 + r2)
	{
		V4 normal =cwt1.getPosition() - cwt2.getPosition();
		normal = normal.normalize();
		return normal;
	}

	return V4(0.0f,0.0f,0.0f,0.0f);
}

V4 SphereBoxCollisionMediator::intersects(const ColliderComponent& collider1, const ColliderComponent& collider2) const
{
	// TODO
	return  V4(0.0f, 0.0f, 0.0f, 0.0f);;
}

V4 SpherePlaneCollisionMediator::intersects(const ColliderComponent& collider1, const ColliderComponent& collider2) const
{
	Mtx cwt = collider1.getWorldTransform();

	float r = 0.5f * V4(cwt[0][0], cwt[1][0], cwt[2][0], 0.0f).length();

	V4 planeEq = static_cast<const PlaneColliderComponent&>(collider2).getEquation();
	float A = planeEq.x, B = planeEq.y, C = planeEq.z, D = planeEq.w;
	float x0 = cwt[3][0], y0 = cwt[3][1], z0= cwt[3][2];
	float d = fabs(A*x0 + B*y0 + C*z0 + D)/ sqrt(A*A + B*B + C*C);
	
	if (d < r)
	{
		V4 normal = V4{ A,B,C,0.0f }.normalize();
		return normal;
	}
		
	return  V4(0.0f, 0.0f, 0.0f, 0.0f);;
}

V4 BoxBoxCollisionMediator::intersects(const ColliderComponent& collider1, const ColliderComponent& collider2) const
{
	// TODO
	return  V4(0.0f, 0.0f, 0.0f, 0.0f);;
}