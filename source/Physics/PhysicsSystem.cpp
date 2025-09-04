#include "PhysicsSystem.h"
#include "PhysicsComponent.h"
#include "ColliderComponent.h"
#include "Engine/TransformComponent.h"

struct PhysicsEntity
{
	PhysicsComponent* physics = nullptr;
	std::vector<ColliderComponent*> colliders;
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
				actor->getComponents<ColliderComponent>()
			);
		}
	});

	for (auto& entity : entities)
	{
		auto physics = entity.physics;
		TransformComponent& transformComponent = physics->getActor()->getTransformComponent();
		V4 velocity = physics->getVelocity();
		V4 acceleration = transformComponent.getTransform().getPosition() * -0.0000025f / physics->getMass();
		velocity += acceleration * dt;
		physics->setVelocity(velocity);
		transformComponent.setTransform(transformComponent.getTransform() * Mtx::translate(velocity * dt));
	}

	for (int i = 0; i < entities.size(); ++i)
		for (int j = i + 1; j < entities.size(); ++j)
		{
			auto& entity1 = entities[i];
			auto& entity2 = entities[j];
			for (auto collider1 : entity1.colliders)
				for (auto collider2 : entity2.colliders)
				{
					if (collider1->intersects(*collider2))
					{
						std::cout << "Collision!" << std::endl;
						// TODO: react on the collision
					}
				}
		}
}