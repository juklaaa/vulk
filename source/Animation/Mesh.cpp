#include "Mesh.h"
#include "Importers/Importer_IQM.h"
#include "Engine/Log.h"

#define FILL_VERTEX(Type, xxx) \
if (vertarray.type == Type) \
{ \
	auto it = floats.begin(); \
	for (auto& v : vertices) \
	{ \
		for (uint j = 0u; j < vertarray.size; ++j) \
			v.xxx[j] = *it++; \
	} \
}

std::vector<Mesh> Mesh::load(std::string_view filepath)
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
				FILL_VERTEX(IQM_VertexArrayType::IQM_TANGENT, tangent);
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
						for (uint j = 0u; j < vertarray.size; ++j) 
							v.weights[j].index = *it++;
					}
				}
				if (vertarray.type == IQM_VertexArrayType::IQM_BLENDWEIGHTS)
				{
					auto it = uchars.begin();
					for (auto& v : vertices)
					{
						for (uint j = 0u; j < vertarray.size; ++j)
							v.weights[j].weight = *it++ / 255.0f;
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
			result.push_back(std::move(mesh));
		}
		
	}
	log(x, x, "Read meshes");
	return result;
}