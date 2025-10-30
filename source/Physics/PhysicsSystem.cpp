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

	for (auto& entity1 : entities)
	{
		Mtx entity1_OriginalTransform;
		auto physics = entity1.physics;
		if (physics->isDynamic())
		{
			TransformComponent& transformComponent = physics->getActor()->getTransformComponent();
			V4 velocity = physics->getVelocity();
			V4 acceleration = V4{ 0.0f, 0.0f,-0.0000025f };
			velocity += acceleration * dt;
			physics->setVelocity(velocity);
			entity1_OriginalTransform = transformComponent.getTransform();
			auto transform = entity1_OriginalTransform;
			transform = transform * Mtx::translate(velocity * dt) * physics->getAngularVelocity() * 0.5f;
			transformComponent.setTransform(transformComponent.getTransform() );
		}

		for (auto& entity2 : entities)
		{
			if (&entity1 == &entity2)
				continue;

			auto&& collided = [&entity1_OriginalTransform](PhysicsEntity& a, PhysicsEntity& b) -> std::optional<V4>
				{
					for (auto collider1 : a.colliders)
						for (auto collider2 : b.colliders)
						{
							if (auto normal = collider1->intersects(*collider2, ColliderComponent::CollisionContext{ entity1_OriginalTransform }))
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

				physics1->getActor()->getTransformComponent().setTransform(entity1_OriginalTransform);

				float e = (physics1->getRestitution() + physics2->getRestitution()) / 2;

				float m1 = physics1->getMass();
				float m2 = physics2->getMass();

				V4 v1 = entity1.physics->getVelocity();
				V4 v2 = entity2.physics->getVelocity();

				V4 n = nOpt.value();
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