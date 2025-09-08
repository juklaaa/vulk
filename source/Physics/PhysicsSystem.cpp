#include "PhysicsSystem.h"
#include "PhysicsComponent.h"
#include "ColliderComponent.h"
#include "Engine/TransformComponent.h"

struct PhysicsEntity
{
	PhysicsComponent* physics = nullptr;
	std::vector<ColliderComponent*> colliders;
	Mtx originalTransform;
};

void PhysicsSystem::update(Scene& scene, float dt)
{
	std::vector<PhysicsEntity> entities;
	scene.forAllActors([&entities](Actor* actor)
	{
		if (auto component = actor->getComponent<PhysicsComponent>())
		{
			entities.emplace_back
			(
				component,
				actor->getComponents<ColliderComponent>(),
				actor->getTransformComponent().getTransform()
			);
		}
	});

	for (auto& entity : entities)
	{
		auto physics = entity.physics;
		TransformComponent& transformComponent = physics->getActor()->getTransformComponent();
		V4 velocity = physics->getVelocity();
		V4 acceleration = transformComponent.getWorldTransform().getPosition() * -0.0000025f * 0.0f / physics->getMass();
		velocity += acceleration * dt;
		physics->setVelocity(velocity);
		transformComponent.setTransform(transformComponent.getTransform() * Mtx::translate(velocity * dt));
	}

	for (int i = 0; i < entities.size(); ++i)
		for (int j = i + 1; j < entities.size(); ++j)
		{
			auto& entity1 = entities[i];
			auto& entity2 = entities[j];
			auto&& collided = [](PhysicsEntity& a, PhysicsEntity& b)
			{
				for (auto collider1 : a.colliders)
					for (auto collider2 : b.colliders)
						if (collider1->intersects(*collider2))
							return true;

				return false;
			};

			if (collided(entity1, entity2))
			{
				entity1.physics->getActor()->getTransformComponent().setTransform(entity1.originalTransform);
				entity2.physics->getActor()->getTransformComponent().setTransform(entity2.originalTransform);

				entity1.physics->setVelocity(entity1.physics->getVelocity() * -1.0f);
				entity2.physics->setVelocity(entity2.physics->getVelocity() * -1.0f);
			}
		}
}