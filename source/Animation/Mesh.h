#pragma once

#include "Common.h"
#include "Engine/Math/Math.h"

class Mesh
{
public:

	static std::vector<Mesh> load(std::string_view filepath);

	struct Vertex
	{
		V4 pos;
		V4 tex;
		V4 normal;
		V4 tangent;
		
		struct BoneWeight
		{
			uchar index = 0u;
			float weight = 0.0f;
		};

		BoneWeight weights[4];
	};

protected:

	std::vector<Vertex> vertices;
	std::vector<ushort> indices;
};
