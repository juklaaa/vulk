#include "model.h"
#include "renderer.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "third_party/tiny_obj_loader.h"


Model::~Model()
{
	assert(!isInitialized);
}

bool Model::load(Renderer* renderer_, std::string_view filename)
{
	renderer = renderer_;

	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename.data()))
	{
		return false;
	}

	std::unordered_map<Vertex, uint32_t> uniqueVertices{};

	for (const auto& shape : shapes)
	{
		for (const auto& index : shape.mesh.indices)
		{
			Vertex vertex{};
			if (index.vertex_index >= 0)
			{
				vertex.pos =
				{
					attrib.vertices[3 * index.vertex_index + 0],
					attrib.vertices[3 * index.vertex_index + 1],
					attrib.vertices[3 * index.vertex_index + 2]
				};

				vertex.color =
				{
					attrib.colors[3 * index.vertex_index + 0],
					attrib.colors[3 * index.vertex_index + 1],
					attrib.colors[3 * index.vertex_index + 2]
				};
			}

			if (index.texcoord_index >= 0)
			{
				vertex.texCoord =
				{
					attrib.texcoords[2 * index.texcoord_index + 0],
					1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
				};
			}

			if (index.normal_index >= 0)
			{
				vertex.normal =
				{
					attrib.normals[3 * index.normal_index + 0],
					attrib.normals[3 * index.normal_index + 1],
					attrib.normals[3 * index.normal_index + 2]
				};
			}

			if (uniqueVertices.count(vertex) == 0)
			{
				uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
				vertices.push_back(vertex);
			}

			indices.push_back(uniqueVertices[vertex]);
		}
	}

	computeTangents();
	createVertexBuffer();
	createIndexBuffer();

	isInitialized = true;

	return true;
}

void Model::unload()
{
	vkDestroyBuffer(getDevice(), indexBuffer, nullptr);
	vkFreeMemory(getDevice(), indexBufferMemory, nullptr);

	vkDestroyBuffer(getDevice(), vertexBuffer, nullptr);
	vkFreeMemory(getDevice(), vertexBufferMemory, nullptr);

	isInitialized = false;
}

VkDevice Model::getDevice() const
{
	return renderer->getDevice();
}

void Model::computeTangents()
{
	for (int i = 0; i < indices.size(); i += 3)
	{
		Vertex& v0 = vertices[indices[i]];
		Vertex& v1 = vertices[indices[i + 1]];
		Vertex& v2 = vertices[indices[i + 2]];

		glm::vec3 edge1 = v1.pos - v0.pos;
		glm::vec3 edge2 = v2.pos - v0.pos;

		float deltaU1 = v1.texCoord.x - v0.texCoord.x;
		float deltaV1 = v1.texCoord.y - v0.texCoord.y;

		float deltaU2 = v2.texCoord.x - v0.texCoord.x;
		float deltaV2 = v2.texCoord.y - v0.texCoord.y;

		float f = 1.0f / (deltaU1 * deltaV2 - deltaU2 * deltaV1);

		glm::vec3 tangent;

		tangent.x = f * (deltaV2 * edge1.x - deltaV1 * edge2.x);
		tangent.y = f * (deltaV2 * edge1.y - deltaV1 * edge2.y);
		tangent.z = f * (deltaV2 * edge1.z - deltaV1 * edge2.z);

		v0.tangent += tangent;
		v1.tangent += tangent;
		v2.tangent += tangent;
	}

	for (int i = 0; i < vertices.size(); i++)
	{
		vertices[i].tangent = glm::normalize(vertices[i].tangent);
	}
}

void Model::createVertexBuffer()
{
	VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	renderer->getImpl().createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(getDevice(), stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, vertices.data(), (size_t)bufferSize);
	vkUnmapMemory(getDevice(), stagingBufferMemory);

	renderer->getImpl().createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);

	renderer->getImpl().copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

	vkDestroyBuffer(getDevice(), stagingBuffer, nullptr);
	vkFreeMemory(getDevice(), stagingBufferMemory, nullptr);
}

void Model::createIndexBuffer()
{
	VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	renderer->getImpl().createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(getDevice(), stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, indices.data(), (size_t)bufferSize);
	vkUnmapMemory(getDevice(), stagingBufferMemory);

	renderer->getImpl().createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);

	renderer->getImpl().copyBuffer(stagingBuffer, indexBuffer, bufferSize);

	vkDestroyBuffer(getDevice(), stagingBuffer, nullptr);
	vkFreeMemory(getDevice(), stagingBufferMemory, nullptr);
}