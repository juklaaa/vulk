#pragma once

#include "Engine/Component.h"
#include "Model.h"
#include "texture.h"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include "Animation/SkelAnimation.h"

class Material
{
public:
	const Texture* getTexture() const { return texture; }
	void setTexture(const Texture* texture_) { texture = texture_; textured = true; }

	const Texture* getNormalMap() const { return normalMap; }
	void setNormalMap(const Texture* normalMap_) { normalMap = normalMap_; }

	int getLightReflection() const { return lightReflection; }
	void setLightReflection(int powerOfTwo) { lightReflection = powerOfTwo; }
	
	glm::vec4 getColor() const { return color;}
	void setColor(glm::vec4 color_) { color = color_; }
	void setColor(float r, float g, float b) { color = glm::vec4(r,g,b, 1.0f); }

	bool isTextured() const { return textured;}

private:
	const Texture* texture = nullptr;
	const Texture* normalMap = nullptr;
	
	glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
	int lightReflection = 0;
	bool textured = false;
};

class VisualComponent : public Component
{
public:

	const Model* getModel() const { return model; }
	VisualComponent* setModel(const Model* model_) { model = model_; return this; }	

	const Material* getMaterial() const { return material; }
	VisualComponent* setMaterial(const Material* material_) { material = material_; return this; }
	
	void playAnimation(const SkelAnimation* animation, const SkelAnimation::Frame* initialFrame);
	void stopAnimation();
	void setAnimationSpeed(float speed) { animationSpeed = speed; }
	const SkelAnimation::Frame* getAnimationFrame() const;
	const SkelAnimation::Frame* getInitialAnimationFrame() const { return initialFrame; }
	
	virtual void tick(float dt) override;

private:
	const Model* model = nullptr;
	const Material* material = nullptr;
	const Skeleton* skeleton = nullptr;
	const SkelAnimation* animation = nullptr;
	const SkelAnimation::Frame* initialFrame = nullptr;
	float time = 0.0f;
	bool isAnimationPlaying = false;
	float animationSpeed = 1.0f;
};
