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
		if (physics->isDynamic())
		{
			TransformComponent& transformComponent = physics->getActor()->getTransformComponent();
			V4 velocity = physics->getVelocity();
			V4 acceleration = V4{ 0.0f, 0.0f,-0.0000025f };
			velocity += acceleration * dt;
			physics->setVelocity(velocity);
			transformComponent.setTransform(transformComponent.getTransform() * Mtx::translate(velocity * dt));
		}
	}

	bool smthCollided = true;
	int t = 0;
	while (smthCollided )
	{
		t++;
		smthCollided = false;
		for (int i = 0; i < entities.size(); ++i)
			for (int j = i + 1; j < entities.size(); ++j)
			{
				auto& entity1 = entities[i];
				auto& entity2 = entities[j];
				auto&& collided = [](PhysicsEntity& a, PhysicsEntity& b) -> std::optional<V4>
					{
						for (auto collider1 : a.colliders)
							for (auto collider2 : b.colliders)
							{
								if (auto normal = collider1->intersects(*collider2))
								{
									return normal;
								}
							}

						return {};
					};

				if (auto nOpt = collided(entity1, entity2))
				{
					auto physics1 = entity1.physics;
					auto physics2 = entity2.physics;

					V4 n = nOpt.value();

					V4 v1 = entity1.physics->getVelocity();
					V4 v2 = entity2.physics->getVelocity();

					V4 pos1 = physics1->getActor()->getTransformComponent().getWorldTransform().getPosition();
					V4 pos2 = physics2->getActor()->getTransformComponent().getWorldTransform().getPosition();

					if ((pos2 - pos1).dot(n) < 0)
						n = { -n.x,-n.y,-n.z,-n.w };

					if (v1.dot(n) -v2.dot(n) < 0 && physics1->isDynamic() && physics2->isDynamic())
						continue;

					smthCollided = true;

					physics1->getActor()->getTransformComponent().setTransform(entity1.originalTransform);
					physics2->getActor()->getTransformComponent().setTransform(entity2.originalTransform);

					float e = (physics1->getRestitution() + physics2->getRestitution()) / 2;

					float m1 = physics1->getMass();
					float m2 = physics2->getMass();
					
					float j = (v1 - v2).dot(n) * (e + 1) * (m1 * m2) / (m1 + m2);

					V4 newV1 = v1 - n * (j / m1);
					V4 newV2 = v2 + n * (j / m2);

					if (physics1->isDynamic())
						physics1->setVelocity(newV1);

					if (physics2->isDynamic())
						physics2->setVelocity(newV2);
				}
			}
	}
}