#pragma once

#include "common.h"

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

class Renderer;
class RendererImpl;

class Texture
{
public:

	Texture() = default;
	~Texture();

	bool load(Renderer* renderer, std::string_view filename, VkFormat format = VkFormat::VK_FORMAT_R8G8B8A8_SRGB);
	void unload();

	VkImageView getImageView() const;

private:

	RendererImpl& getImpl();
	VkDevice getDevice() const;

	Renderer* renderer = nullptr;
	VkImage textureImage;
	VkDeviceMemory textureImageMemory;
	VkImageView textureImageView;
	uint32_t mipLevels = 0u;
	bool isInitialized = false;
};