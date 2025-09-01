#pragma once

#include "Engine/Component.h"
#include "Model.h"
#include "texture.h"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

class Material
{
public:
	const Texture* getTexture() const { return texture; }
	void setTexture(const Texture* texture_) { texture = texture_; textured = true; }

	const Texture* getNormalMap() const { return normalMap; }
	void setNormalMap(const Texture* normalMap_) { normalMap = normalMap_; }

	int getLightReflection() const { return lightReflection; }
	void setLightReflection(int powerOfTwo) { lightReflection = powerOfTwo; }
	
	glm::vec3 getColor() const { return color;}
	void setColor(glm::vec3 color_) { color = color_; }
	void setColor(float r, float g, float b) { color = glm::vec3(r,g,b); }

	bool isTextured() const { return textured;}

private:
	const Texture* texture = nullptr;
	const Texture* normalMap = nullptr;
	
	glm::vec3 color = { 1.0f, 1.0f, 1.0f };
	int lightReflection = 0;
	bool textured = false;
};


class VisualComponent : public Component
{
public:

	const Model* getModel() const { return model; }
	void setModel(const Model* model_) { model = model_; }	

	const Material* getMaterial() const { return material; }
	void setMaterial(const Material* material_) { material = material_; }

protected:

private:
	const Model* model = nullptr;
	const Material* material = nullptr;
	
};

