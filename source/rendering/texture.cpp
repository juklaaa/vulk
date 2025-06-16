#include "texture.h"
#include "renderer.h"

#define STB_IMAGE_IMPLEMENTATION
#include "third_party/stb_image.h"

Texture::~Texture()
{
	assert(!isInitialized);
}

bool Texture::load(Renderer* renderer_, std::string_view filename, VkFormat format/* = VkFormat::VK_FORMAT_R8G8B8A8_SRGB*/)
{
	renderer = renderer_;

	int texWidth, texHeight, texChannels;
	stbi_uc* pixels = stbi_load(filename.data(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	VkDeviceSize imageSize = texWidth * texHeight * 4;

	if (!pixels)
	{
		return false;
	}

	mipLevels = (uint32_t)(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	getImpl().createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(getDevice(), stagingBufferMemory, 0, imageSize, 0, &data);
	memcpy(data, pixels, (size_t)(imageSize));
	vkUnmapMemory(getDevice(), stagingBufferMemory);

	stbi_image_free(pixels);

	getImpl().createImage(texWidth, texHeight, mipLevels, VK_SAMPLE_COUNT_1_BIT, format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory);

	getImpl().transitionImageLayout(textureImage, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels);
	getImpl().copyBufferToImage(stagingBuffer, textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));

	getImpl().generateMipmaps(textureImage, format, texWidth, texHeight, mipLevels);

	vkDestroyBuffer(getDevice(), stagingBuffer, nullptr);
	vkFreeMemory(getDevice(), stagingBufferMemory, nullptr);

	textureImageView = getImpl().createImageView(textureImage, format, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);

	isInitialized = true;

	return true;
}

void Texture::unload()
{
	vkDestroyImageView(getDevice(), textureImageView, nullptr);
	vkDestroyImage(getDevice(), textureImage, nullptr);
	vkFreeMemory(getDevice(), textureImageMemory, nullptr);

	isInitialized = false;
}

VkImageView Texture::getImageView() const
{
	return textureImageView;
}

RendererImpl& Texture::getImpl()
{
	return renderer->getImpl();
}

VkDevice Texture::getDevice() const
{
	return renderer->getDevice();
}
