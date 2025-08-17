#pragma once

#include "Engine/Component.h"
#include "Model.h"
#include "texture.h"

class VisualComponent : public Component
{
public:

	const Model* getModel() const { return model; }
	void setModel(const Model* model_) { model = model_; }

	const Texture* getTexture() const { return texture; }
	void setTexture(const Texture* texture_) { texture = texture_; }

	const Texture* getNormalMap() const { return normalMap; }
	void setNormalMap(const Texture* normalMap_) { normalMap = normalMap_; }


protected:

private:
	const Model* model = nullptr;
	const Texture* texture = nullptr;
	const Texture* normalMap = nullptr;
};

