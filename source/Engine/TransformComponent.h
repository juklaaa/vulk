#pragma once

#include "Component.h"
#include "Math/Math.h"

class TransformComponent : public Component
{
public:

	const Mtx& getTransform() const;
	Mtx& getTransform();
	void setTransform(const Mtx& transform);
	const Mtx& getWorldTransform() const;

private:

	void updateWorldTransform();

	Mtx transform = Mtx::Indentity();
	Mtx worldTransform = Mtx::Indentity();
	bool dirty = true;
};