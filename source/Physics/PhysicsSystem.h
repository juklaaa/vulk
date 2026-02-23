#pragma once

#include "Engine/Scene.h"

class PhysicsComponent;

class PhysicsSystem
{
public:

	void update(Scene& scene, float dt);

protected:
	std::unordered_map<PhysicsComponent*, Mtx> lastFrameTransforms;
};
