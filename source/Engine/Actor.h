#pragma once

#include "Common.h"
#include "Component.h"
#include "TransformComponent.h"

class Scene;

class Actor
{
public:

	Actor();
	virtual ~Actor();

	Scene* getScene() const { return scene; }

	Component* addComponent(Component* component);
	template<typename ComponentType>
	ComponentType* addComponent()
	{
		auto newComp = new ComponentType;
		newComp->setActor(this);
		components.push_back(newComp);
		return newComp;
	}
	//void removeComponent(Component* component);
	TransformComponent& getTransformComponent();

	template<typename ComponentType>
	ComponentType* getComponent() const
	{
		for (Component* component : components)
			if (auto asWanted = dynamic_cast<ComponentType*>(component))
				return asWanted;

		return nullptr;
	}

	template<typename ComponentType>
	std::vector<ComponentType*> getComponents() const
	{
		std::vector<ComponentType*> wantedComponents;
		for (Component* component : components)
			if (auto asWanted = dynamic_cast<ComponentType*>(component))
				wantedComponents.push_back(asWanted);

		return wantedComponents;
	}

	virtual void tick(float dt);

protected:

private:
	
	friend class Scene;

	Scene* scene = nullptr;
	TransformComponent transformComponent;
	std::vector<Component*> components;
};