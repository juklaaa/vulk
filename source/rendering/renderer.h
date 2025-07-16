#pragma once

#include "common.h"
#include "renderer_impl.h"
#include "model.h"
#include "texture.h"

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Renderer
{
public:

	void init(GLFWwindow* window);
	void deinit();
	void drawFrame(bool framebufferResized, bool isPPLightingEnabled);
	void waitUntilDone();

	RendererImpl& getImpl() { return impl; }
	VkDevice getDevice() { return impl.device; }

private:

	struct Pipeline
	{
		void init(Renderer* renderer, std::string_view shaderPath, VkRenderPass renderPass, VkSampleCountFlagBits msaaSamples, int numVertAttributes);
		void deinit();

		void createDescriptorSetLayout();
		void createGraphicsPipeline(std::string_view shaderPath, VkRenderPass renderPass, VkSampleCountFlagBits msaaSamples, int numVertAttributes);

		void createUniformBuffers();
		void createDescriptorPool();
		void createDescriptorSets();
		void updateUniformBuffer(uint32_t currentImage);

		RendererImpl& getImpl() { return renderer->impl; }
		VkDevice getDevice() { return renderer->impl.device; }
		int getNumFramesInFlight() const { return renderer->impl.getNumFramesInFlight(); }

		Renderer* renderer = nullptr;
		VkPipelineLayout pipelineLayout;
		VkPipeline graphicsPipeline;
		VkDescriptorSetLayout descriptorSetLayout;
		std::vector<VkBuffer> uniformBuffers;
		std::vector<VkDeviceMemory> uniformBuffersMemory;
		std::vector<void*> uniformBuffersMapped;
		VkDescriptorPool descriptorPool;
		std::vector<VkDescriptorSet> descriptorSets;
	};

	friend class RendererImpl;

	void createTextureSampler();
	
	void updateUniformBuffer(uint32_t currentImage);
	
	void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
	
	RendererImpl impl;

	Pipeline pipeline;
	Pipeline offscreenPipeline;

	Texture texture;
	Texture normalMap;

	VkSampler textureSampler;

	Model model;
	Model floorModel;
	
	uint32_t mipLevels;
	bool isPPLightingEnabled = true;
};