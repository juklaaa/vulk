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
		Mtx entity1_OriginalWorldTransform;
		auto physics = entity1.physics;
		if (physics->getFlags() & PhysicsComponent::Dynamic)
		{
			TransformComponent& transformComponent = physics->getActor()->getTransformComponent();
			V4 velocity = physics->getVelocity();
			if (physics->getFlags() & PhysicsComponent::Gravity)
			{
				V4 acceleration = V4{ 0.0f, 0.0f,-0.0000025f };
				velocity += acceleration * dt;
			}
			physics->setVelocity(velocity);
			entity1_OriginalTransform = transformComponent.getTransform();
			entity1_OriginalWorldTransform = transformComponent.getWorldTransform();
			auto transform = entity1_OriginalTransform;
			auto angularVelocity = physics->getAngularVelocity();
			transform = transform * Mtx::translate(velocity * dt);
			if (angularVelocity.w != 0.0f)
			{
				V4 pos = transform.getPosition();
				transform = transform * Mtx::translate(pos * -1.0f);
				transform = transform * Mtx::rotate(angularVelocity.xyz(), angularVelocity.w * dt);
				transform = transform * Mtx::translate(pos);
			}
			transformComponent.setTransform(transform);

			for (auto& entity2 : entities)
			{
				if (&entity1 == &entity2)
					continue;

				auto&& collided = [&entity1_OriginalWorldTransform](PhysicsEntity& a, PhysicsEntity& b) -> std::optional<V4>
					{
						for (auto collider1 : a.colliders)
							for (auto collider2 : b.colliders)
							{
								if (auto collision = collider1->intersects(*collider2, ColliderComponent::Context{ entity1_OriginalWorldTransform }))
								{
									return collision->normal;
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

					float invM1 = physics1->getFlags() & PhysicsComponent::Heavy ? 0.0f : 1.0f / physics1->getMass();
					float invM2 = physics2->getFlags() & PhysicsComponent::Heavy ? 0.0f : 1.0f / physics2->getMass();
					assert(invM1 + invM2 != 0.0f);

					V4 v1 = entity1.physics->getVelocity();
					V4 v2 = entity2.physics->getVelocity();

					V4 n = nOpt.value();				

					float j = (v1 - v2).dot(n) * (e + 1) / (n.dot(n) * (invM1 + invM2));

					V4 newV1 = v1 - n * (j * invM1);
					V4 newV2 = v2 + n * (j * invM2);

					if (physics1->getFlags() & PhysicsComponent::Dynamic)
						physics1->setVelocity(newV1);

					if (physics2->getFlags() & PhysicsComponent::Dynamic)
						physics2->setVelocity(newV2);
				}
			}
		}
	}
}