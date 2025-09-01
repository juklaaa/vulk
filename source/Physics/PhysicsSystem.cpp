#include "PhysicsSystem.h"
#include "PhysicsComponent.h"
#include "Engine/TransformComponent.h"

void PhysicsSystem::update(Scene& scene, float dt)
{
	std::vector<PhysicsComponent*> components;
	scene.forAllActors([&components](Actor* actor)
					   {
						   if (auto component = actor->getComponent<PhysicsComponent>())
						   {
							   components.push_back(component);
						   }
					   });

	for (auto component : components)
	{
		TransformComponent& transformComponent = component->getActor()->getTransformComponent();
		V4 velocity = component->getVelocity();
		V4 acceleration = V4{ 0.0f, 0.0f, -0.0000025f } / component->getMass();
		velocity += acceleration * dt;
		component->setVelocity(velocity);
		transformComponent.setTransform(transformComponent.getTransform() * Mtx::translate(velocity * dt));
	}
}