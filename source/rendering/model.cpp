#include "model.h"
#include "renderer.h"




Model::~Model()
{
	assert(!isInitialized);
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

void Model::setMesh(Renderer* renderer_, Mesh* mesh_)
{
	renderer = renderer_;
	mesh = mesh_;

	createVertexBuffer();
	createIndexBuffer();

	isInitialized = true;
}

void Model::createVertexBuffer()
{
	VkDeviceSize bufferSize = sizeof(mesh->vertices[0]) * mesh->vertices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	renderer->getImpl().createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(getDevice(), stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, mesh->vertices.data(), (size_t)bufferSize);
	vkUnmapMemory(getDevice(), stagingBufferMemory);

	renderer->getImpl().createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);

	renderer->getImpl().copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

	vkDestroyBuffer(getDevice(), stagingBuffer, nullptr);
	vkFreeMemory(getDevice(), stagingBufferMemory, nullptr);

}
void Model::createIndexBuffer()
{
	VkDeviceSize bufferSize = sizeof(mesh->indices[0]) * mesh->indices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	renderer->getImpl().createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(getDevice(), stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, mesh->indices.data(), (size_t)bufferSize);
	vkUnmapMemory(getDevice(), stagingBufferMemory);

	renderer->getImpl().createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);

	renderer->getImpl().copyBuffer(stagingBuffer, indexBuffer, bufferSize);

	vkDestroyBuffer(getDevice(), stagingBuffer, nullptr);
	vkFreeMemory(getDevice(), stagingBufferMemory, nullptr);
}