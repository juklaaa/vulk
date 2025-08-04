#pragma once

#include "Component.h"
#include "Math/Math.h"

class TransformComponent : public Component
{
public:

	const Mtx& getTransform() const { return transform; }

protected:

private:
	Mtx transform = Mtx::Indentity();
};