#include "Mesh.h"
#include "Importers/Importer_IQM.h"
#include "Engine/Log.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "third_party/tiny_obj_loader.h"

#define FILL_VERTEX(Type, xxx) \
if (vertarray.type == Type) \
{ \
	auto it = floats.begin(); \
	for (auto& v : vertices) \
	{ \
		for (uint j = 0u; j < vertarray.size; ++j) \
			{\
				if(j==3) continue;\
				v.xxx[j] = *it++;\
			} \
	} \
}

std::vector<Mesh> Mesh::loadiqm(std::string_view filepath)
{
	std::vector<Mesh> result;
	std::ifstream f(filepath.data(), std::ios_base::binary);
	if (!f)
		return result;

	iqmheader header;
	f.read(reinterpret_cast<char*>(&header), sizeof(header));

	std::vector<Mesh::Vertex> vertices;
	std::vector<uint> indices;

	if (header.num_meshes > 0 && header.ofs_meshes > 0)
	{
		vertices.resize(header.num_vertexes);
		indices.resize(header.num_triangles * 3);
		assert(header.num_vertexarrays > 0);
		f.seekg(header.ofs_vertexarrays);
		std::vector<iqmvertexarray> vertexarrays;
		vertexarrays.resize(header.num_vertexarrays);
		f.read(reinterpret_cast<char*>(vertexarrays.data()), sizeof(iqmvertexarray) * header.num_vertexarrays);
		for (auto& vertarray : vertexarrays)
		{
			f.seekg(vertarray.offset);
			
			std::vector<float> floats;
			std::vector<uchar> uchars;
			uint numElements = vertarray.size * header.num_vertexes;

			if (vertarray.format == IQM_Format::IQM_FLOAT)
			{
				floats.resize(numElements);
				f.read(reinterpret_cast<char*>(floats.data()), numElements * sizeof(float));
				FILL_VERTEX(IQM_VertexArrayType::IQM_POSITION, pos);
				FILL_VERTEX(IQM_VertexArrayType::IQM_TEXCOORD, tex);
				FILL_VERTEX(IQM_VertexArrayType::IQM_NORMAL, normal);
				FILL_VERTEX(IQM_VertexArrayType::IQM_TANGENT, tangent); //TO DO
			}
			else if (vertarray.format == IQM_Format::IQM_UBYTE)
			{
				uchars.resize(numElements);
				f.read(reinterpret_cast<char*>(uchars.data()), numElements * sizeof(char));
				if (vertarray.type == IQM_VertexArrayType::IQM_BLENDINDEXES)
				{
					auto it = uchars.begin();
					for (auto& v : vertices) 
					{ 
						v.boneIndices[0] = *it++;
						v.boneIndices[1] = *it++;
						v.boneIndices[2] = *it++;
						v.boneIndices[3] = *it++;
					}
				}
				if (vertarray.type == IQM_VertexArrayType::IQM_BLENDWEIGHTS)
				{
					auto it = uchars.begin();
					for (auto& v : vertices)
					{
						v.weights.x = *it++ / 255.0f;
						v.weights.y = *it++ / 255.0f;
						v.weights.z = *it++ / 255.0f;
						v.weights.w = *it++ / 255.0f;
					}
				}
			}
			else
				assert(false && "Unsupported type");
		}

		f.seekg(header.ofs_triangles);
		f.read(reinterpret_cast<char*>(indices.data()), indices.size() * sizeof(uint));

		result.reserve(header.num_meshes);
		for (uint i = 0; i < header.num_meshes; ++i)
		{
			iqmmesh meshIQM;
			f.seekg(header.ofs_meshes);
			f.read(reinterpret_cast<char*>(&meshIQM), sizeof(meshIQM));

			Mesh mesh;
			std::copy_n(vertices.begin() + meshIQM.first_vertex, meshIQM.num_vertexes, std::back_inserter(mesh.vertices));
			std::copy_n(indices.begin() + meshIQM.first_triangle * 3, meshIQM.num_triangles * 3, std::back_inserter(mesh.indices));
			// TODO: recalculate indices accounting to their local vertexes


			int s = mesh.indices.size()-1;
			for (int i = 0; i <= s/2;i++)
			{ 
				std::swap(mesh.indices[i], mesh.indices[s - i]);
			}
			result.push_back(std::move(mesh));
		}
		
	}

	
	log(x, Info, "Read meshes");
	return result;
}

std::vector<Mesh> Mesh::loadobj(std::string_view filepath)
{
	std::vector<Mesh> result;

	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;

	assert(tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filepath.data()));

	//std::unordered_map<Vertex, uint32_t> uniqueVertices{};//TO DO

	for (const auto& shape : shapes)
	{
		Mesh mesh;
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
				vertex.tex =
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

			/*if (uniqueVertices.count(vertex) == 0)
			{
				uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
				
			}*/

			mesh.vertices.push_back(vertex);
			mesh.indices.push_back(mesh.vertices.size()-1);
		}
		mesh.computeTangents();
		result.push_back(mesh);
	}

	return result;
}

void Mesh::generatePlane(float size)
{
	float x = size / 2.0f;
	float y = size / 2.0f;

	V3 positions[4] =
	{
		{ x,  y,  0}, {-x,  y,  0}, {-x, -y,  0}, { x, -y,  0}
	};
	V3 normal = { 0, 0, 1 };

	V2 tex[4] = { {1,1}, {1,0}, {0,0}, {0,1} };

	for (int i = 0; i < 4; i++) {
		Vertex v{};
		v.pos = positions[i];
		v.color = { 1,1,1 };
		v.normal = normal;
		v.tex = tex[i];
		vertices.push_back(v);
	}

	indices.push_back(0);
	indices.push_back(1);
	indices.push_back(2);

	indices.push_back(2);
	indices.push_back(3);
	indices.push_back(0);

	computeTangents();
}

void Mesh::generateCube( float size)
{
	float x = size / 2.0f;
	float y = size / 2.0f;
	float z = size / 2.0f;

	V3 positions[6][4] =
	{
		{ { x,  y,  z}, {-x,  y,  z}, {-x, -y,  z}, { x, -y,  z} },
		{ { x,  y, -z}, { x, -y, -z}, {-x, -y, -z}, {-x,  y, -z} },
		{ {-x,  y,  z}, {-x,  y, -z}, {-x, -y, -z}, {-x, -y,  z} },
		{ { x,  y,  z}, { x, -y,  z}, { x, -y, -z}, { x,  y, -z} },
		{ { x,  y,  z}, { x,  y, -z}, {-x,  y, -z}, {-x,  y,  z} },
		{ { x, -y,  z}, {-x, -y,  z}, {-x, -y, -z}, { x, -y, -z} }
	};

	V3 normals[6] =
	{
		{ 0, 0, 1},
		{ 0, 0,-1},
		{-1, 0, 0},
		{ 1, 0, 0},
		{ 0, 1, 0},
		{ 0,-1, 0}
	};

	V2 tex[4] = { {1,1}, {1,0}, {0,0}, {0,1} };

	for (int f = 0; f < 6; f++) {
		for (int i = 0; i < 4; i++) {
			Vertex v;
			v.pos = positions[f][i];
			v.color = { 1,1,1 };
			v.normal = normals[f];
			v.tex = tex[i];
			vertices.push_back(v);
		}
	}

	for (int f = 0; f < 6; f++) {
		int start = f * 4;
		indices.push_back(start + 0);
		indices.push_back(start + 1);
		indices.push_back(start + 2);

		indices.push_back(start + 2);
		indices.push_back(start + 3);
		indices.push_back(start + 0);
	}

	computeTangents();
}

void Mesh::generateSphere(float radius, int segments, int rings)
{

	for (int i = 0; i <= rings; i++)
	{
		float theta = PI * i / rings;

		for (int j = 0; j <= segments; j++)
		{
			float phi = PI * 2.0f * j / segments;

			Vertex v;
			v.pos.x = radius * sin(theta) * cos(phi);
			v.pos.y = radius * sin(theta) * sin(phi);
			v.pos.z = radius * cos(theta);

			v.color = V3(1.0f, 1.0f, 1.0f);
			v.normal = v.pos.normalize();
			v.tex = V2(float(i) / rings, float(j) / segments);

			vertices.push_back(v);

		}
	}

	for (int i = 0; i < rings; i++)
	{
		for (int j = 0; j < segments; j++)
		{
			int first = i * (segments + 1) + j;
			int second = first + segments + 1;

			indices.push_back(first);
			indices.push_back(second);
			indices.push_back(first + 1);

			indices.push_back(second);
			indices.push_back(second + 1);
			indices.push_back(first + 1);
		}
	}


	computeTangents();
}

void Mesh::computeTangents()
{
	for (int i = 0; i < indices.size(); i += 3)
	{
		Vertex& v0 = vertices[indices[i]];
		Vertex& v1 = vertices[indices[i + 1]];
		Vertex& v2 = vertices[indices[i + 2]];

		V3 edge1 = v1.pos - v0.pos;
		V3 edge2 = v2.pos - v0.pos;

		float deltaU1 = v1.tex.x - v0.tex.x;
		float deltaV1 = v1.tex.y - v0.tex.y;

		float deltaU2 = v2.tex.x - v0.tex.x;
		float deltaV2 = v2.tex.y - v0.tex.y;

		float f = 1.0f / (deltaU1 * deltaV2 - deltaU2 * deltaV1);

		V3 tangent;

		tangent.x = f * (deltaV2 * edge1.x - deltaV1 * edge2.x);
		tangent.y = f * (deltaV2 * edge1.y - deltaV1 * edge2.y);
		tangent.z = f * (deltaV2 * edge1.z - deltaV1 * edge2.z);

		v0.tangent += tangent;
		v1.tangent += tangent;
		v2.tangent += tangent;
	}

	for (int i = 0; i < vertices.size(); i++)
	{
		vertices[i].tangent = vertices[i].tangent.normalize();
	}
}