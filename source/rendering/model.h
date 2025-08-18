#pragma once

#include "common.h"

#include <vulkan/vulkan.h>
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

class Renderer;

struct Vertex
{
	glm::vec3 pos;
	glm::vec3 color;
	glm::vec2 texCoord;
	glm::vec3 normal;
	glm::vec3 tangent = glm::vec3(0.0f);

	static VkVertexInputBindingDescription getBindingDescription()
	{
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions(int numAttributes)
	{
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions;

		VkFormat formats[] = { VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R32G32_SFLOAT, VK_FORMAT_R32G32B32_SFLOAT,  VK_FORMAT_R32G32B32_SFLOAT };
		uint32_t offsets[] = { offsetof(Vertex, pos), offsetof(Vertex, color), offsetof(Vertex, texCoord), offsetof(Vertex, normal), offsetof(Vertex, tangent) };

		for (int i = 0; i < numAttributes; ++i)
		{
			auto& descr = attributeDescriptions.emplace_back();
			memset(&descr, 0, sizeof(VkVertexInputAttributeDescription));
			descr.binding = 0;
			descr.location = i;
			descr.format = formats[i];
			descr.offset = offsets[i];
		}

		return attributeDescriptions;
	}

	bool operator==(const Vertex& other) const
	{
		return pos == other.pos && color == other.color && texCoord == other.texCoord && normal == other.normal && tangent == other.tangent;
	}
};

namespace std
{
	template<> struct hash<Vertex>
	{
		size_t operator()(Vertex const& vertex) const
		{
			return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^ ((hash<glm::vec2>()(vertex.texCoord) << 1) ^ (hash<glm::vec2>()(vertex.normal) >> 1));
		}
	};
}

class Model
{
public:

	Model() = default;
	~Model();

	bool load(Renderer* renderer, std::string_view filename);
	void unload();

	VkBuffer getVertexBuffer() const { return vertexBuffer; }
	VkBuffer getIndexBuffer() const { return indexBuffer; }
	uint32_t getNumIndices() const { return (uint32_t)indices.size(); }

	void loadPlane(Renderer* renderer, float size = 1.0f);
	void loadCube(Renderer* renderer, float size = 1.0f);
	void loadSphere(Renderer* renderer, float size = 1.0f);


protected:

	VkDevice getDevice() const;

	void computeTangents();
	void createVertexBuffer();
	void createIndexBuffer();

	Renderer* renderer = nullptr;
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;
	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;
	bool isInitialized = false;
};

