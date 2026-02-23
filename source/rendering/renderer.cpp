#include "Renderer.h"
#include "Engine/Scene.h"
#include "VisualComponent.h"
#include "Animation/SkelAnimation.h"
#include "Engine/Log.h"

const std::string TEXTURE_PATH = "textures/deafult_texture.jpg";
const std::string NORMAL_MAP_PATH = "textures/deafult_normal.png";

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
	glm::vec4 light;
	glm::vec4 modelColor;
	float modelLightReflection;
	float textured;
	float padding1;
	float padding2;
};

constexpr uint NUM_BONES = 64u;
struct AnimUBO : UBO
{
	glm::vec4 initialPoseBonePositions[NUM_BONES];
	glm::vec4 initialPoseBoneRotations[NUM_BONES];
	glm::vec4 initialPoseBoneScales[NUM_BONES];
	
	glm::vec4 poseBonePositions[NUM_BONES];
	glm::vec4 poseBoneRotations[NUM_BONES];
	glm::vec4 poseBoneScales[NUM_BONES];
};

struct AnimOffscreenUBO : OffscreenUBO
{
	glm::vec4 initialPoseBonePositions[NUM_BONES];
	glm::vec4 initialPoseBoneRotations[NUM_BONES];
	glm::vec4 initialPoseBoneScales[NUM_BONES];

	glm::vec4 poseBonePositions[NUM_BONES];
	glm::vec4 poseBoneRotations[NUM_BONES];
	glm::vec4 poseBoneScales[NUM_BONES];
};

struct SceneDataForUniforms
{
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;
	glm::mat4 offscreenMVP;
	glm::vec3 light;
	const Material* material = nullptr;
	
	const SkelAnimation::Frame* frame = nullptr;
	const SkelAnimation::Frame* initialFrame = nullptr;
};

template<typename UBO_Type>
struct Pipeline : public Renderer::PipelineBase
{
	virtual size_t getUBOSize() const override
	{
		return sizeof(UBO_Type);
	}
};

template<typename UBO_Type = UBO>
struct MainPipeline : public Pipeline<UBO_Type>
{
	virtual void fillUBO(const void* sceneDataRaw, void* uboRaw)
	{
		const SceneDataForUniforms& sceneData = *reinterpret_cast<const SceneDataForUniforms*>(sceneDataRaw);
		UBO_Type& ubo = *reinterpret_cast<UBO_Type*>(uboRaw);
		
		ubo.model = sceneData.model;
		ubo.view = sceneData.view;
		ubo.proj = sceneData.proj;
		ubo.depthMVP = sceneData.offscreenMVP;
		ubo.light = glm::vec4((sceneData.light), 0);

		if (auto material = sceneData.material)
		{
			ubo.modelColor = material->getColor();
			ubo.modelLightReflection = material->getLightReflection();
			ubo.textured = material->isTextured();
		}
		else
		{
			ubo.modelColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
			ubo.modelLightReflection = 0;
			ubo.textured = 1;
		}
	}
	
	virtual void createDescriptorSetLayout() override
	{
		VkDescriptorSetLayoutBinding uboLayoutBinding{};
		uboLayoutBinding.binding = 0;
		uboLayoutBinding.descriptorCount = 1;
		uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboLayoutBinding.pImmutableSamplers = nullptr;
		uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

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

		VkDescriptorSetLayout& descSetLayout = Renderer::PipelineBase::descriptorSetLayout;
		if (vkCreateDescriptorSetLayout(Renderer::PipelineBase::getDevice(), &layoutInfo, nullptr, &descSetLayout) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create descriptor set layout!");
		}
	}

	virtual void createDescriptorPool(int maxNumVisuals) override
	{
		std::array<VkDescriptorPoolSize, 4> poolSizes{};
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[0].descriptorCount = maxNumVisuals * Renderer::PipelineBase::getNumFramesInFlight();

		poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[1].descriptorCount = maxNumVisuals * Renderer::PipelineBase::getNumFramesInFlight();

		poolSizes[2].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[2].descriptorCount = maxNumVisuals * Renderer::PipelineBase::getNumFramesInFlight();

		poolSizes[3].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[3].descriptorCount = maxNumVisuals * Renderer::PipelineBase::getNumFramesInFlight();

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = poolSizes.size();
		poolInfo.pPoolSizes = poolSizes.data();
		poolInfo.maxSets = maxNumVisuals * Renderer::PipelineBase::getNumFramesInFlight();

		VkDescriptorPool& descPool = Renderer::PipelineBase::descriptorPool;
		if (vkCreateDescriptorPool(Renderer::PipelineBase::getDevice(), &poolInfo, nullptr, &descPool) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create descriptor pool!");
		}
	}

	virtual void createDescriptorSets(int numVisuals, int currentImage, const std::vector<VisualComponent*>& visualComponents) override
	{
		if (Renderer::PipelineBase::numVisuals_DescriptorSets[currentImage] == numVisuals)
			return;

		auto& set = Renderer::PipelineBase::descriptorSets[currentImage];
		if (!set.empty())
			vkFreeDescriptorSets(Renderer::PipelineBase::getDevice(), Renderer::PipelineBase::descriptorPool, set.size(), set.data());

		Renderer::PipelineBase::numVisuals_DescriptorSets[currentImage] = numVisuals;

		uint32_t numDescriptorSets = numVisuals;

		std::vector<VkDescriptorSetLayout> layouts(numDescriptorSets, Renderer::PipelineBase::descriptorSetLayout);
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = Renderer::PipelineBase::descriptorPool;
		allocInfo.descriptorSetCount = static_cast<uint32_t>(numDescriptorSets);
		allocInfo.pSetLayouts = layouts.data();

		set.resize(numDescriptorSets);
		if (vkAllocateDescriptorSets(Renderer::PipelineBase::getDevice(), &allocInfo, set.data()) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to allocate descriptor sets!");
		}

		for (size_t i = 0; i < numDescriptorSets; i++)
		{
			VkDescriptorBufferInfo bufferInfo{};
			bufferInfo.buffer = Renderer::PipelineBase::uniformBuffers[currentImage][i];
			bufferInfo.offset = 0;
			bufferInfo.range = Pipeline<UBO_Type>::getUBOSize();

			auto actorMaterial = visualComponents[i]->getActor()->getComponent<VisualComponent>()->getMaterial();

			VkDescriptorImageInfo imageInfo{};	
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			if (actorMaterial == nullptr)
				imageInfo.imageView = Renderer::PipelineBase::renderer->texture.getImageView();
			else
			{
				auto actorTexture = actorMaterial->getTexture();
				if(actorTexture==nullptr)
					imageInfo.imageView = Renderer::PipelineBase::renderer->texture.getImageView();
				else
					imageInfo.imageView = actorTexture->getImageView();
			}				
			imageInfo.sampler = Renderer::PipelineBase::renderer->textureSampler;

			VkDescriptorImageInfo normalMapInfo{};	
			normalMapInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			if (actorMaterial == nullptr)
				normalMapInfo.imageView = Renderer::PipelineBase::renderer->normalMap.getImageView();
			else
			{
				auto actorNomal = actorMaterial->getNormalMap();
				if (actorNomal == nullptr)
					normalMapInfo.imageView = Renderer::PipelineBase::renderer->normalMap.getImageView();
				else
					normalMapInfo.imageView = actorNomal->getImageView();
			}
			normalMapInfo.sampler = Renderer::PipelineBase::renderer->textureSampler;

			VkDescriptorImageInfo depthMapInfo{};
			depthMapInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
			depthMapInfo.imageView = Renderer::PipelineBase::getImpl().shadowmapDepthImageView;
			depthMapInfo.sampler = Renderer::PipelineBase::renderer->textureSampler;

			std::array<VkWriteDescriptorSet, 4> descriptorWrites{};

			descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[0].dstSet = set[i];
			descriptorWrites[0].dstBinding = 0;
			descriptorWrites[0].dstArrayElement = 0;
			descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[0].descriptorCount = 1;
			descriptorWrites[0].pBufferInfo = &bufferInfo;

			descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[1].dstSet = set[i];
			descriptorWrites[1].dstBinding = 1;
			descriptorWrites[1].dstArrayElement = 0;
			descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrites[1].descriptorCount = 1;
			descriptorWrites[1].pImageInfo = &imageInfo;

			descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[2].dstSet = set[i];
			descriptorWrites[2].dstBinding = 2;
			descriptorWrites[2].dstArrayElement = 0;
			descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrites[2].descriptorCount = 1;
			descriptorWrites[2].pImageInfo = &normalMapInfo;

			descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[3].dstSet = set[i];
			descriptorWrites[3].dstBinding = 3;
			descriptorWrites[3].dstArrayElement = 0;
			descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrites[3].descriptorCount = 1;
			descriptorWrites[3].pImageInfo = &depthMapInfo;

			vkUpdateDescriptorSets(Renderer::PipelineBase::getDevice(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
		}
	}
};

struct AnimPipeline : public MainPipeline<AnimUBO>
{
	virtual void fillUBO(const void* sceneDataRaw, void* uboRaw)
	{
		const SceneDataForUniforms& sceneData = *reinterpret_cast<const SceneDataForUniforms*>(sceneDataRaw);
		AnimUBO& ubo = *reinterpret_cast<AnimUBO*>(uboRaw);
		MainPipeline<AnimUBO>::fillUBO(&sceneData, &ubo);
		FillPosePSR(ubo.initialPoseBonePositions, ubo.initialPoseBoneRotations, ubo.initialPoseBoneScales, sceneData.initialFrame);
		FillPosePSR(ubo.poseBonePositions, ubo.poseBoneRotations, ubo.poseBoneScales, sceneData.frame);
	}
	
	void FillPosePSR(glm::vec4* positions, glm::vec4* rotations, glm::vec4* scales, const SkelAnimation::Frame* frame)
	{
		if (!frame)
			return;
		
		for (int i = 0; i < NUM_BONES; ++i)
			rotations[i] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
		
		int i = 0;
		for (auto& bone : frame->bones)
		{
			positions[i] = glm::vec4(bone.position.x, bone.position.y, bone.position.z, 0.0f);
			rotations[i] = glm::vec4(bone.rotation.x, bone.rotation.y, bone.rotation.z, bone.rotation.w);
			scales[i] = glm::vec4(bone.size.x, bone.size.y, bone.size.z, 0.0f);
			++i;
			assert(i < NUM_BONES);
		}	
	}
};

struct OffscreenPipeline : public Pipeline<OffscreenUBO>
{
	virtual void fillUBO(const void* sceneDataRaw, void* uboRaw) override
	{
		const SceneDataForUniforms& sceneData = *reinterpret_cast<const SceneDataForUniforms*>(sceneDataRaw);
		OffscreenUBO& ubo = *reinterpret_cast<OffscreenUBO*>(uboRaw);
		ubo.MVP = sceneData.offscreenMVP;
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
		layoutInfo.bindingCount = 1;
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
		poolSize.descriptorCount = maxNumVisuals * getNumFramesInFlight();

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = 1;
		poolInfo.pPoolSizes = &poolSize;
		poolInfo.maxSets = maxNumVisuals * getNumFramesInFlight();

		if (vkCreateDescriptorPool(getDevice(), &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create descriptor pool!");
		}
	}

	virtual void createDescriptorSets(int numVisuals, int currentImage, const std::vector<VisualComponent*>& visualComponents) override
	{
		if (numVisuals_DescriptorSets[currentImage] == numVisuals)
			return;

		auto& set = descriptorSets[currentImage];
		if (!set.empty())
			vkFreeDescriptorSets(getDevice(), descriptorPool, set.size(), set.data());

		numVisuals_DescriptorSets[currentImage] = numVisuals;

		uint32_t numDescriptorSets = numVisuals;

		std::vector<VkDescriptorSetLayout> layouts(numDescriptorSets, descriptorSetLayout);
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = descriptorPool;
		allocInfo.descriptorSetCount = static_cast<uint32_t>(numDescriptorSets);
		allocInfo.pSetLayouts = layouts.data();

		set.resize(numDescriptorSets);
		if (vkAllocateDescriptorSets(getDevice(), &allocInfo, set.data()) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to allocate descriptor sets!");
		}

		for (size_t i = 0; i < numDescriptorSets; i++)
		{
			VkDescriptorBufferInfo bufferInfo{};
			bufferInfo.buffer = uniformBuffers[currentImage][i];
			bufferInfo.offset = 0;
			bufferInfo.range = getUBOSize();

			std::array<VkWriteDescriptorSet, 1> descriptorWrites{};

			descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[0].dstSet = set[i];
			descriptorWrites[0].dstBinding = 0;
			descriptorWrites[0].dstArrayElement = 0;
			descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[0].descriptorCount = 1;
			descriptorWrites[0].pBufferInfo = &bufferInfo;

			vkUpdateDescriptorSets(getDevice(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
		}
	}
};

struct AnimOffscreenPipeline : public Pipeline<AnimOffscreenUBO>
{
	virtual void fillUBO(const void* sceneDataRaw, void* uboRaw) override
	{
		const SceneDataForUniforms& sceneData = *reinterpret_cast<const SceneDataForUniforms*>(sceneDataRaw);
		AnimOffscreenUBO& ubo = *reinterpret_cast<AnimOffscreenUBO*>(uboRaw);
		ubo.MVP = sceneData.offscreenMVP;

		FillPosePSR(ubo.initialPoseBonePositions, ubo.initialPoseBoneRotations, ubo.initialPoseBoneScales, sceneData.initialFrame);
		FillPosePSR(ubo.poseBonePositions, ubo.poseBoneRotations, ubo.poseBoneScales, sceneData.frame);
	}

	void FillPosePSR(glm::vec4* positions, glm::vec4* rotations, glm::vec4* scales, const SkelAnimation::Frame* frame)
	{
		if (!frame)
			return;

		for (int i = 0; i < NUM_BONES; ++i)
			rotations[i] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

		int i = 0;
		for (auto& bone : frame->bones)
		{
			positions[i] = glm::vec4(bone.position.x, bone.position.y, bone.position.z, 0.0f);
			rotations[i] = glm::vec4(bone.rotation.x, bone.rotation.y, bone.rotation.z, bone.rotation.w);
			scales[i] = glm::vec4(bone.size.x, bone.size.y, bone.size.z, 0.0f);
			++i;
			assert(i < NUM_BONES);
		}
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
		layoutInfo.bindingCount = 1;
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
		poolSize.descriptorCount = maxNumVisuals * getNumFramesInFlight();

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = 1;
		poolInfo.pPoolSizes = &poolSize;
		poolInfo.maxSets = maxNumVisuals * getNumFramesInFlight();

		if (vkCreateDescriptorPool(getDevice(), &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create descriptor pool!");
		}
	}

	virtual void createDescriptorSets(int numVisuals, int currentImage, const std::vector<VisualComponent*>& visualComponents) override
	{
		if (numVisuals_DescriptorSets[currentImage] == numVisuals)
			return;

		auto& set = descriptorSets[currentImage];
		if (!set.empty())
			vkFreeDescriptorSets(getDevice(), descriptorPool, set.size(), set.data());

		numVisuals_DescriptorSets[currentImage] = numVisuals;

		uint32_t numDescriptorSets = numVisuals;

		std::vector<VkDescriptorSetLayout> layouts(numDescriptorSets, descriptorSetLayout);
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = descriptorPool;
		allocInfo.descriptorSetCount = static_cast<uint32_t>(numDescriptorSets);
		allocInfo.pSetLayouts = layouts.data();

		set.resize(numDescriptorSets);
		if (vkAllocateDescriptorSets(getDevice(), &allocInfo, set.data()) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to allocate descriptor sets!");
		}

		for (size_t i = 0; i < numDescriptorSets; i++)
		{
			VkDescriptorBufferInfo bufferInfo{};
			bufferInfo.buffer = uniformBuffers[currentImage][i];
			bufferInfo.offset = 0;
			bufferInfo.range = getUBOSize();

			std::array<VkWriteDescriptorSet, 1> descriptorWrites{};

			descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[0].dstSet = set[i];
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

	pipeline = std::make_unique<MainPipeline<>>();
	pipeline->init(this, "nmap", impl.renderPass, impl.msaaSamples, 5);
	animPipeline = std::make_unique<AnimPipeline>();
	animPipeline->init(this, "anim", "nmap", impl.renderPass, impl.msaaSamples, 7);
	offscreenPipeline = std::make_unique<OffscreenPipeline>();
	offscreenPipeline->init(this, "offscreen", impl.shadowmapRenderPass, VK_SAMPLE_COUNT_1_BIT, 1);
	animOffscreenPipeline = std::make_unique<AnimOffscreenPipeline>();
	animOffscreenPipeline->init(this, "animOffscreen", "offscreen", impl.shadowmapRenderPass, VK_SAMPLE_COUNT_1_BIT, 7);
}

void Renderer::deinit()
{
	vkDestroySampler(getDevice(), textureSampler, nullptr);

	pipeline->deinit();
	animPipeline->deinit();
	offscreenPipeline->deinit();
	animOffscreenPipeline->deinit();

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
		auto worldTransform = visual->getActor()->getTransformComponent().getTransform();
		glm::mat4 model;
		static_assert(sizeof(Mtx) == sizeof(glm::mat4));
		memcpy(&model, &worldTransform, sizeof(Mtx));
		glm::mat4 view = glm::lookAt(cameraPos, cameraLookAt, glm::vec3(0.0f, 0.0f, 1.0f));
		glm::mat4 proj = glm::perspective(glm::radians(45.0f), getImpl().swapChainExtent.width / (float)getImpl().swapChainExtent.height, 0.1f, 20.0f);

		proj[1][1] *= -1;

		glm::vec3 light = glm::vec3(-5.0f, -3.0f, 5.0f);

		float orthoSize = 10.0f;
		float nearPlane = 0.1f;
		float farPlane = 20.0f;

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
		sceneDataForUniforms.material = visual->getMaterial();
		sceneDataForUniforms.frame = visual->getAnimationFrame();
		sceneDataForUniforms.initialFrame = visual->getInitialAnimationFrame();
		sceneDatas.push_back(sceneDataForUniforms);
	}

	pipeline->createUniformBuffers(visualComponents.size(), currentImage);
	pipeline->createDescriptorSets(visualComponents.size(), currentImage, visualComponents);
	pipeline->updateUniformBuffer(currentImage, sceneDatas.data(), visualComponents.size());

	animPipeline->createUniformBuffers(visualComponents.size(), currentImage);
	animPipeline->createDescriptorSets(visualComponents.size(), currentImage, visualComponents);
	animPipeline->updateUniformBuffer(currentImage, sceneDatas.data(), visualComponents.size());
	
	offscreenPipeline->createUniformBuffers(visualComponents.size(), currentImage);
	offscreenPipeline->createDescriptorSets(visualComponents.size(), currentImage, visualComponents);
	offscreenPipeline->updateUniformBuffer(currentImage, sceneDatas.data(), visualComponents.size());

	animOffscreenPipeline->createUniformBuffers(visualComponents.size(), currentImage);
	animOffscreenPipeline->createDescriptorSets(visualComponents.size(), currentImage, visualComponents);
	animOffscreenPipeline->updateUniformBuffer(currentImage, sceneDatas.data(), visualComponents.size());
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

void Renderer::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t swapchainImageIndex, const std::vector<VisualComponent*>& visualComponents)
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
		renderPassInfo.framebuffer = impl.shadowmapFramebuffer;
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = { RendererImpl::shadowmapSize, RendererImpl::shadowmapSize };

		std::array<VkClearValue, 1> clearValues{};
		clearValues[0].depthStencil = { 1.0f, 0 };

		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

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
			auto vis = visualComponents[i];
			if (!vis->getInitialAnimationFrame())
			{
				vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, offscreenPipeline->graphicsPipeline);
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, offscreenPipeline->pipelineLayout, 0, 1, &offscreenPipeline->descriptorSets[impl.currentFrame][i], 0, nullptr);
				PushConstants constants;
				constants.isPPLightingEnabled = isPPLightingEnabled;
				vkCmdPushConstants(commandBuffer, offscreenPipeline->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstants), &constants);
			}
			else
			{
				vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, animOffscreenPipeline->graphicsPipeline);
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, animOffscreenPipeline->pipelineLayout, 0, 1, &animOffscreenPipeline->descriptorSets[impl.currentFrame][i], 0, nullptr);
				PushConstants constants;
				constants.isPPLightingEnabled = isPPLightingEnabled;
				vkCmdPushConstants(commandBuffer, animOffscreenPipeline->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstants), &constants);
			}

			VkBuffer vertexBuffers[] = { vis->getModel()->getVertexBuffer() };
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

			vkCmdBindIndexBuffer(commandBuffer, vis->getModel()->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

			vkCmdDrawIndexed(commandBuffer, vis->getModel()->getNumIndices(), 1, 0, 0, 0);
		}

		vkCmdEndRenderPass(commandBuffer);
	}

	{
		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = impl.renderPass;
		renderPassInfo.framebuffer = impl.swapChainFramebuffers[swapchainImageIndex];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = impl.swapChainExtent;

		std::array<VkClearValue, 2> clearValues{};
		clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
		clearValues[1].depthStencil = { 1.0f, 0 };

		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);


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
			auto vis = visualComponents[i];
			if (!vis->getInitialAnimationFrame())
			{
				vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->graphicsPipeline);
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipelineLayout, 0, 1, &pipeline->descriptorSets[impl.currentFrame][i], 0, nullptr);
				PushConstants constants;
				constants.isPPLightingEnabled = isPPLightingEnabled;
				vkCmdPushConstants(commandBuffer, pipeline->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstants), &constants);
			}
			else
			{
				vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, animPipeline->graphicsPipeline);
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, animPipeline->pipelineLayout, 0, 1, &animPipeline->descriptorSets[impl.currentFrame][i], 0, nullptr);
				PushConstants constants;
				constants.isPPLightingEnabled = isPPLightingEnabled;
				vkCmdPushConstants(commandBuffer, animPipeline->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstants), &constants);
			}
			
			VkBuffer vertexBuffers[] = { vis->getModel()->getVertexBuffer()};
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

			vkCmdBindIndexBuffer(commandBuffer, vis->getModel()->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

			vkCmdDrawIndexed(commandBuffer, vis->getModel()->getNumIndices(), 1, 0, 0, 0);
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
	init(renderer_, shaderPath, shaderPath, renderPass, msaaSamples, numVertAttributes);
}

void Renderer::PipelineBase::init(Renderer* renderer_, std::string_view vertShaderPath, std::string_view fragShaderPath, VkRenderPass renderPass, VkSampleCountFlagBits msaaSamples, int numVertAttributes)
{
	renderer = renderer_;

	memset(numVisuals_Uniforms, 0, sizeof(numVisuals_Uniforms));
	memset(numVisuals_DescriptorSets, 0, sizeof(numVisuals_DescriptorSets));

	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = getUBOSize();
	bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VkBuffer sampleBuffer;
	if (vkCreateBuffer(getDevice(), &bufferInfo, nullptr, &sampleBuffer) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create buffer!");
	}

	vkGetBufferMemoryRequirements(getDevice(), sampleBuffer, &uniformMemReq);
	vkDestroyBuffer(getDevice(), sampleBuffer, nullptr);

	createDescriptorSetLayout();
	createGraphicsPipeline(vertShaderPath, fragShaderPath, renderPass, msaaSamples, numVertAttributes);
	constexpr int maxNumVisuals = 100;
	allocateUniformBuffersMemory(maxNumVisuals);
	createDescriptorPool(maxNumVisuals);
}

void Renderer::PipelineBase::deinit()
{
	for (size_t i = 0; i < getImpl().getNumFramesInFlight(); i++)
	{

		for (auto buffer : uniformBuffers[i])
			vkDestroyBuffer(getDevice(), buffer, nullptr);
		vkFreeMemory(getDevice(), uniformBuffersMemory[i], nullptr);
	}

	vkDestroyDescriptorPool(getDevice(), descriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(getDevice(), descriptorSetLayout, nullptr);

	vkDestroyPipeline(getDevice(), graphicsPipeline, nullptr);
	vkDestroyPipelineLayout(getDevice(), pipelineLayout, nullptr);
}

void Renderer::PipelineBase::createGraphicsPipeline(std::string_view shaderPath, VkRenderPass renderPass, VkSampleCountFlagBits msaaSamples, int numVertAttributes)
{
	createGraphicsPipeline(shaderPath, shaderPath, renderPass, msaaSamples, numVertAttributes);
}

void Renderer::PipelineBase::createGraphicsPipeline(std::string_view vertShaderPath, std::string_view fragShaderPath, VkRenderPass renderPass, VkSampleCountFlagBits msaaSamples, int numVertAttributes)
{
	auto vertShaderCode = readFile(std::format("shaders/{}_v.spv", vertShaderPath));
	auto fragShaderCode = readFile(std::format("shaders/{}_f.spv", fragShaderPath));

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
	for (int currentImage = 0; currentImage < getImpl().getNumFramesInFlight(); currentImage++)
	{
		uint32_t memSize = uniformMemReq.size * maxNumVisuals;
		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memSize;
		allocInfo.memoryTypeIndex = getImpl().findMemoryType(uniformMemReq.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		if (vkAllocateMemory(getDevice(), &allocInfo, nullptr, &uniformBuffersMemory[currentImage]) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to allocate buffer memory!");
		}
		vkMapMemory(getDevice(), uniformBuffersMemory[currentImage], 0, memSize, 0, &uniformBuffersMapped[currentImage]);
	}
}

void Renderer::PipelineBase::createUniformBuffers(int numVisuals, int frameIndex)
{
	if (numVisuals_Uniforms[frameIndex] == numVisuals)
		return;

	auto& uniforms = uniformBuffers[frameIndex];
	for (auto buffer : uniforms)
		vkDestroyBuffer(getDevice(), buffer, nullptr);

	numVisuals_Uniforms[frameIndex] = numVisuals;

	VkDeviceSize bufferSize = getUBOSize();

	uniforms.resize(numVisuals);

	for (size_t i = 0; i < numVisuals; i++)
	{
		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = bufferSize;
		bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateBuffer(getDevice(), &bufferInfo, nullptr, &uniforms[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create buffer!");
		}

		vkBindBufferMemory(getDevice(), uniforms[i], uniformBuffersMemory[frameIndex], i * uniformMemReq.size);
	}
}

void Renderer::PipelineBase::updateUniformBuffer(uint32_t currentImage, const void* sceneDataForUniforms, int numVisuals)
{
	const SceneDataForUniforms* sceneData = reinterpret_cast<const SceneDataForUniforms*>(sceneDataForUniforms);
	char* uboMemory = new char[getUBOSize()];
	for (int i = 0; i < numVisuals; i++)
	{
		memset(uboMemory, 0, getUBOSize());
		fillUBO(&sceneData[i], uboMemory);
		memcpy(reinterpret_cast<char*>(uniformBuffersMapped[currentImage]) + uniformMemReq.size * i, uboMemory, getUBOSize());
	}
	delete[] uboMemory;
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