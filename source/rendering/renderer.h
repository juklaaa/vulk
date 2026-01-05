#pragma once

#include "Common.h"
#include "RendererImpl.h"
#include "Model.h"
#include "Texture.h"
#include "VisualComponent.h"

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>


class Scene;

class Renderer
{
public:

	void init(GLFWwindow* window);
	void deinit();
	void drawFrame(Scene& scene, bool framebufferResized, bool isPPLightingEnabled);
	void waitUntilDone();

	RendererImpl& getImpl() { return impl; }
	VkDevice getDevice() { return impl.device; }

	struct PipelineBase
	{
		virtual ~PipelineBase() = default;

		void init(Renderer* renderer, std::string_view shaderPath, VkRenderPass renderPass, VkSampleCountFlagBits msaaSamples, int numVertAttributes);
		void init(Renderer* renderer, std::string_view vertShaderPath, std::string_view fragShaderPath, VkRenderPass renderPass, VkSampleCountFlagBits msaaSamples, int numVertAttributes);
		void deinit();

		virtual void createDescriptorSetLayout() = 0;
		void createGraphicsPipeline(std::string_view vertShaderPath, std::string_view fragShaderPath, VkRenderPass renderPass, VkSampleCountFlagBits msaaSamples, int numVertAttributes);
		void createGraphicsPipeline(std::string_view shaderPath, VkRenderPass renderPass, VkSampleCountFlagBits msaaSamples, int numVertAttributes);

		void allocateUniformBuffersMemory(int maxNumVisuals);
		void createUniformBuffers(int numVisuals, int currentImage);
		virtual void createDescriptorPool(int maxNumVisuals) = 0;
		virtual void createDescriptorSets(int numVisuals, int currentImage, const std::vector<VisualComponent*>& visualComponents) = 0;

		virtual size_t getUBOSize() const = 0;
		void updateUniformBuffer(uint32_t currentImage, const void* sceneDataForUniforms, int numVisuals);
		virtual void fillUBO(const void* sceneData, void* ubo) = 0;

		RendererImpl& getImpl() { return renderer->impl; }
		VkDevice getDevice() { return renderer->impl.device; }
		uint getNumFramesInFlight() const { return renderer->impl.getNumFramesInFlight(); }

		Renderer* renderer = nullptr;
		VkPipelineLayout pipelineLayout;
		VkPipeline graphicsPipeline;
		VkDescriptorSetLayout descriptorSetLayout;
		std::vector<VkBuffer> uniformBuffers[RendererImpl::getNumFramesInFlightStatic()];
		VkDeviceMemory uniformBuffersMemory[RendererImpl::getNumFramesInFlightStatic()];
		void* uniformBuffersMapped[RendererImpl::getNumFramesInFlightStatic()];
		VkDescriptorPool descriptorPool;
		std::vector<VkDescriptorSet> descriptorSets[RendererImpl::getNumFramesInFlightStatic()];
		int numVisuals_Uniforms[RendererImpl::getNumFramesInFlightStatic()];
		int numVisuals_DescriptorSets[RendererImpl::getNumFramesInFlightStatic()];
		VkMemoryRequirements uniformMemReq;
	};

	Texture texture;
	Texture normalMap;
	VkSampler textureSampler;
private:

	friend class RendererImpl;

	void createTextureSampler();
	
	void updateUniformBuffer(uint32_t currentImage, const std::vector<VisualComponent*>& visualComponents);
	
	void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t swapchainImageIndex, const std::vector<VisualComponent*>& visualComponents);
	
	RendererImpl impl;

	std::unique_ptr<PipelineBase> pipeline;
	std::unique_ptr<PipelineBase> animPipeline;
	std::unique_ptr<PipelineBase> offscreenPipeline;
	
	uint32_t mipLevels;
	bool isPPLightingEnabled = true;
};