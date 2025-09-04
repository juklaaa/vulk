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
	virtual bool intersects(const ColliderComponent& collider1, const ColliderComponent& collider2) const = 0;
};

class GeneralCollisionMediator : public CollisionMediator
{
public:

	GeneralCollisionMediator();
	~GeneralCollisionMediator();
	virtual bool intersects(const ColliderComponent& collider1, const ColliderComponent& collider2) const override;

	static GeneralCollisionMediator& getSingleton();

private:

	std::unordered_map<std::pair<ColliderComponent::Type, ColliderComponent::Type>, CollisionMediator*> mediators;
};

bool ColliderComponent::intersects(ColliderComponent& other) const
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
	virtual bool intersects(const ColliderComponent& collider1, const ColliderComponent& collider2) const override;
};

class SphereBoxCollisionMediator : public CollisionMediator
{
	virtual bool intersects(const ColliderComponent& collider1, const ColliderComponent& collider2) const override;
};

class BoxBoxCollisionMediator : public CollisionMediator
{
	virtual bool intersects(const ColliderComponent& collider1, const ColliderComponent& collider2) const override;
};

GeneralCollisionMediator::GeneralCollisionMediator()
{
	mediators[{ColliderComponent::Type::Sphere, ColliderComponent::Type::Sphere}] = new SphereSphereCollisionMediator;
	mediators[{ColliderComponent::Type::Sphere, ColliderComponent::Type::Box}] = new SphereSphereCollisionMediator;
	mediators[{ColliderComponent::Type::Box, ColliderComponent::Type::Box}] = new BoxBoxCollisionMediator;
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

bool GeneralCollisionMediator::intersects(const ColliderComponent& collider1, const ColliderComponent& collider2) const
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

bool SphereSphereCollisionMediator::intersects(const ColliderComponent& collider1, const ColliderComponent& collider2) const
{
	Mtx cwt1 = collider1.getWorldTransform();
	Mtx cwt2 = collider2.getWorldTransform();
	float r1 = (cwt1 * V4{ 1.0f, 0.0f, 0.0f, 0.0f }).length();
	float r2 = (cwt1 * V4{ 1.0f, 0.0f, 0.0f, 0.0f }).length();
	if (cwt1.getPosition().dist(cwt2.getPosition()) < r1 + r2)
		return true;

	return false;
}

bool SphereBoxCollisionMediator::intersects(const ColliderComponent& collider1, const ColliderComponent& collider2) const
{
	// TODO
	return false;
}

bool BoxBoxCollisionMediator::intersects(const ColliderComponent& collider1, const ColliderComponent& collider2) const
{
	// TODO
	return false;
}