#include "Renderer.h"
#include "Engine/Scene.h"
#include "VisualComponent.h"

const std::string MODEL_PATH = "models/stanford_bunny.obj";
const std::string TEXTURE_PATH = "textures/stanford_bunny.jpg";
const std::string NORMAL_MAP_PATH = "textures/stanford_bunny_normal.png";

std::vector<char> readFile(const std::string& filename);

struct PushConstants
{
	uint32_t isPPLightingEnabled = true;
};

struct OffscreenUBO
{
	glm::mat4 MVP;
};

struct UBO
{
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;
	glm::mat4 depthMVP;
	glm::vec3 light;
};

struct SceneDataForUniforms
{
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;
	glm::mat4 offscreenMVP;
	glm::vec3 light;
};

template<typename UBO_Type>
struct Pipeline : public Renderer::PipelineBase
{
	virtual size_t getUBOSize() const override
	{
		return sizeof(UBO_Type);
	}
};

struct MainPipeline : public Pipeline<UBO>
{
	virtual void updateUniformBuffer(uint32_t currentImage, const void* sceneDataForUniforms, int numVisuals) override
	{
		const SceneDataForUniforms& sceneData = *reinterpret_cast<const SceneDataForUniforms*>(sceneDataForUniforms);
		std::vector<UBO> ubos;
		ubos.reserve(numVisuals);
		for (int i = 0; i < numVisuals; i++)
		{
			UBO ubo{};
			ubo.model = sceneData.model;
			ubo.view = sceneData.view;
			ubo.proj = sceneData.proj;
			ubo.light = sceneData.light;
			ubo.depthMVP = sceneData.offscreenMVP;
			ubos.push_back(ubo);
		}
		size_t dataSize = sizeof(UBO) * ubos.size();
		memcpy(reinterpret_cast<char*>(uniformBuffersMapped) + currentImage * dataSize, ubos.data(), dataSize);
	}

	virtual void createDescriptorSetLayout() override
	{
		VkDescriptorSetLayoutBinding uboLayoutBinding{};
		uboLayoutBinding.binding = 0;
		uboLayoutBinding.descriptorCount = 1;
		uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboLayoutBinding.pImmutableSamplers = nullptr;
		uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		VkDescriptorSetLayoutBinding samplerLayoutBinding{};
		samplerLayoutBinding.binding = 1;
		samplerLayoutBinding.descriptorCount = 1;
		samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		samplerLayoutBinding.pImmutableSamplers = nullptr;
		samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorSetLayoutBinding samplers[] = { samplerLayoutBinding, samplerLayoutBinding, samplerLayoutBinding };
		samplers[1].binding = 2;
		samplers[2].binding = 3;

		std::array<VkDescriptorSetLayoutBinding, 4> bindings = { uboLayoutBinding, samplers[0], samplers[1], samplers[2] };
		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings = bindings.data();

		if (vkCreateDescriptorSetLayout(getDevice(), &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create descriptor set layout!");
		}
	}

	virtual void createDescriptorPool(int maxNumVisuals) override
	{
		std::array<VkDescriptorPoolSize, 4> poolSizes{};
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[0].descriptorCount = static_cast<uint32_t>(getNumFramesInFlight());

		poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[1].descriptorCount = static_cast<uint32_t>(getNumFramesInFlight());

		poolSizes[2].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[2].descriptorCount = static_cast<uint32_t>(getNumFramesInFlight());

		poolSizes[3].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[3].descriptorCount = static_cast<uint32_t>(getNumFramesInFlight());

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		poolInfo.pPoolSizes = poolSizes.data();
		poolInfo.maxSets = static_cast<uint32_t>(maxNumVisuals * getNumFramesInFlight());

		if (vkCreateDescriptorPool(getDevice(), &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create descriptor pool!");
		}
	}

	virtual void createDescriptorSets(int numVisuals) override
	{
		if (numVisuals_DescriptorSets == numVisuals)
			return;

		vkFreeDescriptorSets(getDevice(), descriptorPool, descriptorSets.size(), descriptorSets.data());

		numVisuals_DescriptorSets = numVisuals;

		uint32_t numDescriptorSets = numVisuals * getNumFramesInFlight();

		std::vector<VkDescriptorSetLayout> layouts(numDescriptorSets, descriptorSetLayout);
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = descriptorPool;
		allocInfo.descriptorSetCount = static_cast<uint32_t>(numDescriptorSets);
		allocInfo.pSetLayouts = layouts.data();

		descriptorSets.resize(numDescriptorSets);
		if (vkAllocateDescriptorSets(getDevice(), &allocInfo, descriptorSets.data()) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to allocate descriptor sets!");
		}

		for (size_t i = 0; i < numDescriptorSets; i++)
		{
			VkDescriptorBufferInfo bufferInfo{};
			bufferInfo.buffer = uniformBuffers[i];
			bufferInfo.offset = 0;
			bufferInfo.range = getUBOSize();

			VkDescriptorImageInfo imageInfo{};
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo.imageView = renderer->texture.getImageView();
			imageInfo.sampler = renderer->textureSampler;

			VkDescriptorImageInfo normalMapInfo{};
			normalMapInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			normalMapInfo.imageView = renderer->normalMap.getImageView();
			normalMapInfo.sampler = renderer->textureSampler;

			VkDescriptorImageInfo depthMapInfo{};
			depthMapInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
			depthMapInfo.imageView = getImpl().shadowmapDepthImageView;
			depthMapInfo.sampler = renderer->textureSampler;

			std::array<VkWriteDescriptorSet, 4> descriptorWrites{};

			descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[0].dstSet = descriptorSets[i];
			descriptorWrites[0].dstBinding = 0;
			descriptorWrites[0].dstArrayElement = 0;
			descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[0].descriptorCount = 1;
			descriptorWrites[0].pBufferInfo = &bufferInfo;

			descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[1].dstSet = descriptorSets[i];
			descriptorWrites[1].dstBinding = 1;
			descriptorWrites[1].dstArrayElement = 0;
			descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrites[1].descriptorCount = 1;
			descriptorWrites[1].pImageInfo = &imageInfo;

			descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[2].dstSet = descriptorSets[i];
			descriptorWrites[2].dstBinding = 2;
			descriptorWrites[2].dstArrayElement = 0;
			descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrites[2].descriptorCount = 1;
			descriptorWrites[2].pImageInfo = &normalMapInfo;

			descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[3].dstSet = descriptorSets[i];
			descriptorWrites[3].dstBinding = 3;
			descriptorWrites[3].dstArrayElement = 0;
			descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrites[3].descriptorCount = 1;
			descriptorWrites[3].pImageInfo = &depthMapInfo;

			vkUpdateDescriptorSets(getDevice(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
		}
	}
};

struct OffscreenPipeline : public Pipeline<OffscreenUBO>
{
	virtual void updateUniformBuffer(uint32_t currentImage, const void* sceneDataForUniforms, int numVisuals) override
	{
		const SceneDataForUniforms& sceneData = *reinterpret_cast<const SceneDataForUniforms*>(sceneDataForUniforms);
		std::vector<OffscreenUBO> ubos;
		ubos.reserve(numVisuals);
		for (int i = 0; i < numVisuals; i++)
		{
			ubos.emplace_back(sceneData.offscreenMVP);
		}
		size_t dataSize = sizeof(OffscreenUBO) * ubos.size();
		memcpy(reinterpret_cast<char*>(uniformBuffersMapped) + currentImage * dataSize, ubos.data(), dataSize);
	}

	virtual void createDescriptorSetLayout() override
	{
		VkDescriptorSetLayoutBinding uboLayoutBinding{};
		uboLayoutBinding.binding = 0;
		uboLayoutBinding.descriptorCount = 1;
		uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboLayoutBinding.pImmutableSamplers = nullptr;
		uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount =1;
		layoutInfo.pBindings = &uboLayoutBinding;

		if (vkCreateDescriptorSetLayout(getDevice(), &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create descriptor set layout!");
		}
	}

	virtual void createDescriptorPool(int maxNumVisuals) override
	{
		VkDescriptorPoolSize poolSize{};
		poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSize.descriptorCount = static_cast<uint32_t>(getNumFramesInFlight());

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = 1;
		poolInfo.pPoolSizes = &poolSize;
		poolInfo.maxSets = static_cast<uint32_t>(maxNumVisuals * getNumFramesInFlight());

		if (vkCreateDescriptorPool(getDevice(), &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create descriptor pool!");
		}
	}

	virtual void createDescriptorSets(int numVisuals) override
	{
		if (numVisuals_DescriptorSets == numVisuals)
			return;

		vkFreeDescriptorSets(getDevice(), descriptorPool, descriptorSets.size(), descriptorSets.data());

		numVisuals_DescriptorSets = numVisuals;

		uint32_t numDescriptorSets = numVisuals * getNumFramesInFlight();

		std::vector<VkDescriptorSetLayout> layouts(getNumFramesInFlight(), descriptorSetLayout);
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = descriptorPool;
		allocInfo.descriptorSetCount = numDescriptorSets;
		allocInfo.pSetLayouts = layouts.data();

		descriptorSets.resize(numDescriptorSets);
		if (vkAllocateDescriptorSets(getDevice(), &allocInfo, descriptorSets.data()) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to allocate descriptor sets!");
		}

		for (size_t i = 0; i < numDescriptorSets; i++)
		{
			VkDescriptorBufferInfo bufferInfo{};
			bufferInfo.buffer = uniformBuffers[i];
			bufferInfo.offset = 0;
			bufferInfo.range = getUBOSize();

			std::array<VkWriteDescriptorSet, 1> descriptorWrites{};

			descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[0].dstSet = descriptorSets[i];
			descriptorWrites[0].dstBinding = 0;
			descriptorWrites[0].dstArrayElement = 0;
			descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[0].descriptorCount = 1;
			descriptorWrites[0].pBufferInfo = &bufferInfo;

			vkUpdateDescriptorSets(getDevice(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
		}
	}
};

void Renderer::init(GLFWwindow* window)
{
	impl.init(this, window);
	
	createTextureSampler();
	texture.load(this, TEXTURE_PATH.c_str());
	normalMap.load(this, NORMAL_MAP_PATH.c_str(), VK_FORMAT_R8G8B8A8_UNORM);

	pipeline = std::make_unique<MainPipeline>();
	pipeline->init(this, "nmap", impl.renderPass, impl.msaaSamples, 5);
	offscreenPipeline = std::make_unique<OffscreenPipeline>();
	offscreenPipeline->init(this, "offscreen", impl.shadowmapRenderPass, VK_SAMPLE_COUNT_1_BIT, 1);
}

void Renderer::deinit()
{
	vkDestroySampler(getDevice(), textureSampler, nullptr);

	pipeline->deinit();
	offscreenPipeline->deinit();

	texture.unload();
	normalMap.unload();

	impl.deinit();
}

void Renderer::createTextureSampler()
{
	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

	VkPhysicalDeviceProperties properties{};
	vkGetPhysicalDeviceProperties(impl.physicalDevice, &properties);

	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;

	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;

	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = VK_LOD_CLAMP_NONE;

	if (vkCreateSampler(getDevice(), &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create texture sampler!");
	}
}

void Renderer::updateUniformBuffer(uint32_t currentImage, const std::vector<VisualComponent*>& visualComponents)
{
	static auto startTime = std::chrono::high_resolution_clock::now();
	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

	std::vector<SceneDataForUniforms> sceneDatas;
	sceneDatas.reserve(visualComponents.size());
	for (auto visual : visualComponents)
	{
		glm::mat4 model = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::translate(model, glm::vec3(0.0f, -1.0f, 0.0f));
		model = glm::rotate(model, time * glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		glm::mat4 view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		glm::mat4 proj = glm::perspective(glm::radians(45.0f), getImpl().swapChainExtent.width / (float)getImpl().swapChainExtent.height, 0.1f, 10.0f);

		proj[1][1] *= -1;

		glm::vec3 light = glm::vec3(-5.0f, 5.0f, 5.0f);

		float orthoSize = 4.0f;
		float nearPlane = 0.1f;
		float farPlane = 10.0f;

		glm::vec3 lightDir = glm::normalize(light);
		glm::mat4 lightView = glm::lookAt(lightDir * 5.0f, glm::vec3(0.0f), glm::vec3(0, 0, 1));
		glm::mat4 lightProj = glm::ortho(-orthoSize, orthoSize, -orthoSize, orthoSize, nearPlane, farPlane);

		glm::mat4 offscreenMVP = lightProj * lightView * model;

		SceneDataForUniforms sceneDataForUniforms{};
		sceneDataForUniforms.model = model;
		sceneDataForUniforms.view = view;
		sceneDataForUniforms.proj = proj;
		sceneDataForUniforms.light = light;
		sceneDataForUniforms.offscreenMVP = offscreenMVP;
		sceneDatas.push_back(sceneDataForUniforms);
	}

	pipeline->createUniformBuffers(visualComponents.size());
	pipeline->createDescriptorSets(visualComponents.size());
	pipeline->updateUniformBuffer(currentImage, sceneDatas.data(), visualComponents.size());

	offscreenPipeline->createUniformBuffers(visualComponents.size());
	offscreenPipeline->createDescriptorSets(visualComponents.size());
	offscreenPipeline->updateUniformBuffer(currentImage, sceneDatas.data(), visualComponents.size());
}

void Renderer::waitUntilDone()
{
	vkDeviceWaitIdle(getDevice());
}

void Renderer::drawFrame(Scene& scene, bool framebufferResized, bool isPPLightingEnabled_)
{
	isPPLightingEnabled = isPPLightingEnabled_;

	std::vector<VisualComponent*> visualComponents;
	scene.forAllActors([&visualComponents](Actor* actor)
					   {
						   if (auto visualComponent = actor->getComponent<VisualComponent>())
						   {
							   visualComponents.push_back(visualComponent);
						   }
					   });
	impl.drawFrame(visualComponents, framebufferResized);
}

void Renderer::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, const std::vector<VisualComponent*>& visualComponents)
{
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = 0; // Optional
	beginInfo.pInheritanceInfo = nullptr; // Optional

	if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to begin recording command buffer!");
	}

	{
		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = impl.shadowmapRenderPass;
		renderPassInfo.framebuffer = impl.shadowmapFramebuffer;//to sie gdzieœ usuwa i nie jest odbudowane
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = { RendererImpl::shadowmapSize, RendererImpl::shadowmapSize };

		std::array<VkClearValue, 1> clearValues{};
		clearValues[0].depthStencil = { 1.0f, 0 };

		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, offscreenPipeline->graphicsPipeline);

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(RendererImpl::shadowmapSize);
		viewport.height = static_cast<float>(RendererImpl::shadowmapSize);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = { RendererImpl::shadowmapSize, RendererImpl::shadowmapSize };
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

		for (int i = 0; i < visualComponents.size(); ++i)
		{
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, offscreenPipeline->pipelineLayout, 0, 1, &offscreenPipeline->descriptorSets[impl.currentFrame * visualComponents.size() + i], 0, nullptr);

			auto vis = visualComponents[i];

			PushConstants constants;
			constants.isPPLightingEnabled = isPPLightingEnabled;
			vkCmdPushConstants(commandBuffer, pipeline->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstants), &constants);

			VkBuffer vertexBuffers[] = { vis->getModel().getVertexBuffer() };
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

			vkCmdBindIndexBuffer(commandBuffer, vis->getModel().getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

			vkCmdDrawIndexed(commandBuffer, vis->getModel().getNumIndices(), 1, 0, 0, 0);
		}

		vkCmdEndRenderPass(commandBuffer);
	}

	{
		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = impl.renderPass;
		renderPassInfo.framebuffer = impl.swapChainFramebuffers[imageIndex];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = impl.swapChainExtent;

		std::array<VkClearValue, 2> clearValues{};
		clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
		clearValues[1].depthStencil = { 1.0f, 0 };

		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->graphicsPipeline);

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(impl.swapChainExtent.width);
		viewport.height = static_cast<float>(impl.swapChainExtent.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = impl.swapChainExtent;
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

		for (int i = 0; i < visualComponents.size(); ++i)
		{
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipelineLayout, 0, 1, &pipeline->descriptorSets[impl.currentFrame * visualComponents.size() + i], 0, nullptr);

			auto vis = visualComponents[i];

			PushConstants constants;
			constants.isPPLightingEnabled = isPPLightingEnabled;
			vkCmdPushConstants(commandBuffer, pipeline->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstants), &constants);

			VkBuffer vertexBuffers[] = { vis->getModel().getVertexBuffer()};
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

			vkCmdBindIndexBuffer(commandBuffer, vis->getModel().getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

			vkCmdDrawIndexed(commandBuffer, vis->getModel().getNumIndices(), 1, 0, 0, 0);
		}

		vkCmdEndRenderPass(commandBuffer);
	}

	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to record command buffer!");
	}
}

//==========================Pipeline=========================================//

void Renderer::PipelineBase::init(Renderer* renderer_, std::string_view shaderPath, VkRenderPass renderPass, VkSampleCountFlagBits msaaSamples, int numVertAttributes)
{
	renderer = renderer_;
	createDescriptorSetLayout();
	createGraphicsPipeline(shaderPath, renderPass, msaaSamples, numVertAttributes);
	constexpr int maxNumVisuals = 100;
	allocateUniformBuffersMemory(maxNumVisuals);
	createDescriptorPool(maxNumVisuals);
}

void Renderer::PipelineBase::deinit()
{
	for (size_t i = 0; i < getImpl().getNumFramesInFlight(); i++)
	{
		vkDestroyBuffer(getDevice(), uniformBuffers[i], nullptr);
	}

	vkFreeMemory(getDevice(), uniformBuffersMemory, nullptr);

	vkDestroyDescriptorPool(getDevice(), descriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(getDevice(), descriptorSetLayout, nullptr);

	vkDestroyPipeline(getDevice(), graphicsPipeline, nullptr);
	vkDestroyPipelineLayout(getDevice(), pipelineLayout, nullptr);
}

void Renderer::PipelineBase::createGraphicsPipeline(std::string_view shaderPath, VkRenderPass renderPass, VkSampleCountFlagBits msaaSamples, int numVertAttributes)
{
	auto vertShaderCode = readFile(std::format("shaders/{}_v.spv", shaderPath));
	auto fragShaderCode = readFile(std::format("shaders/{}_f.spv", shaderPath));

	VkShaderModule vertShaderModule = getImpl().createShaderModule(vertShaderCode);
	VkShaderModule fragShaderModule = getImpl().createShaderModule(fragShaderCode);

	VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	auto bindingDescription = Vertex::getBindingDescription();
	auto attributeDescriptions = Vertex::getAttributeDescriptions(numVertAttributes);

	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	std::vector<VkDynamicState> dynamicStates =
	{
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	dynamicState.pDynamicStates = dynamicStates.data();

	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;

	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f; // Optional
	rasterizer.depthBiasClamp = 0.0f; // Optional
	rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.rasterizationSamples = msaaSamples;
	multisampling.pSampleMask = nullptr; // Optional
	multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
	multisampling.alphaToOneEnable = VK_FALSE; // Optional
	multisampling.sampleShadingEnable = VK_TRUE;
	multisampling.minSampleShading = .2f;


	VkPipelineDepthStencilStateCreateInfo depthStencil{};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.minDepthBounds = 0.0f; // Optional
	depthStencil.maxDepthBounds = 1.0f; // Optional
	depthStencil.stencilTestEnable = VK_FALSE;
	depthStencil.front = {}; // Optional
	depthStencil.back = {}; // Optional

	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f; // Optional
	colorBlending.blendConstants[1] = 0.0f; // Optional
	colorBlending.blendConstants[2] = 0.0f; // Optional
	colorBlending.blendConstants[3] = 0.0f; // Optional

	VkPushConstantRange pushConstant{};
	pushConstant.size = sizeof(PushConstants);
	pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges = &pushConstant;

	if (vkCreatePipelineLayout(getDevice(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create pipeline layout!");
	}

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;

	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = &dynamicState;

	pipelineInfo.layout = pipelineLayout;
	pipelineInfo.renderPass = renderPass;
	pipelineInfo.subpass = 0;

	if (vkCreateGraphicsPipelines(getDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create graphics pipeline!");
	}

	getImpl().destroyShaderModule(fragShaderModule);
	getImpl().destroyShaderModule(vertShaderModule);
}

void Renderer::PipelineBase::allocateUniformBuffersMemory(int maxNumVisuals)
{
	uint32_t memSize = getUBOSize() * maxNumVisuals * getNumFramesInFlight();
	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memSize;
	allocInfo.memoryTypeIndex = getImpl().findMemoryType(0xffffffff, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	if (vkAllocateMemory(getDevice(), &allocInfo, nullptr, &uniformBuffersMemory) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate buffer memory!");
	}
	vkMapMemory(getDevice(), uniformBuffersMemory, 0, memSize, 0, &uniformBuffersMapped);
}

void Renderer::PipelineBase::createUniformBuffers(int numVisuals)
{
	if (numVisuals_Uniforms == numVisuals)
		return;

	for (size_t i = 0; i < numVisuals_Uniforms * getNumFramesInFlight(); i++)
	{
		vkDestroyBuffer(getDevice(), uniformBuffers[i], nullptr);
	}

	numVisuals_Uniforms = numVisuals;

	VkDeviceSize bufferSize = getUBOSize();

	uniformBuffers.resize(numVisuals * getNumFramesInFlight());

	for (size_t i = 0; i < numVisuals * getNumFramesInFlight(); i++)
	{
		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = bufferSize;
		bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateBuffer(getDevice(), &bufferInfo, nullptr, &uniformBuffers[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create buffer!");
		}

		vkBindBufferMemory(getDevice(), uniformBuffers[i], uniformBuffersMemory, i * bufferSize);
	}
}

//===========================MISC======================================/
std::vector<char> readFile(const std::string& filename)
{
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open())
	{
		throw std::runtime_error("failed to open file!");
	}

	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);

	file.close();

	return buffer;
}