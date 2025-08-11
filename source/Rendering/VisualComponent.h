#pragma once

#include "Engine/Component.h"
#include "Model.h"

class VisualComponent : public Component
{
public:

	const Model* getModel() const { return model; }
	void setModel(const Model* model_) { model = model_; }

protected:

private:
	const Model* model = nullptr;
};