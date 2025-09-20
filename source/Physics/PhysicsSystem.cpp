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
		V4 acceleration = V4{ 0.0f, 0.0f,-0.0000025f } / physics->getMass();
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
					{
						V4 normal = collider1->intersects(*collider2);
						if (normal != V4(0.0f, 0.0f, 0.0f, 0.0f))
						{
							return normal;
						}
					}													

				return V4(0.0f, 0.0f, 0.0f, 0.0f);
			};

			V4 n = collided(entity1, entity2);//TODO colision normal

			if (n != V4(0.0f, 0.0f, 0.0f, 0.0f))
			{
				entity1.physics->getActor()->getTransformComponent().setTransform(entity1.originalTransform);
				entity2.physics->getActor()->getTransformComponent().setTransform(entity2.originalTransform);			
				
				n = n.normalize();

				auto physics1 = entity1.physics;
				auto physics2 = entity2.physics;

				float e = (physics1->getRestitution() + physics2->getRestitution())/2;

				float m1 = physics1->getMass();
				float m2 = physics2->getMass();
				
				V4 v1 = entity1.physics->getVelocity();
				V4 v2 = entity2.physics->getVelocity();

				V4 newV1 = v1 -   n * ((v1 - v2).dot(n)) * (((1 + e) * m2) / (m1 + m2));
				V4 newV2 = v2 + n * ((v1 - v2).dot(n)) * (((1 + e) * m1) / (m1 + m2));

				entity1.physics->setVelocity(newV1);
				entity2.physics->setVelocity(newV2);
			}
		}
}