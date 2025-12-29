#pragma once

#include "Common.h"
#include "Engine/Math/Math.h"


class Model;

class Mesh
{
public:

	static std::vector<Mesh> load(std::string_view filepath);

	struct Vertex
	{
		V3 pos;
		V3 color = { 1.0f,1.0f,1.0f};
		V2 tex;
		V3 normal;
		V3 tangent;
		
		V4 weightIndices = V4::zero();
		V4 weights = V4::zero();
	};

protected:
	friend class Model;

	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
};
