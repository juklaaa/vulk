#pragma once

#include "Common.h"
#include "Engine/Math/Math.h"


class Model;

class Mesh
{
public:

	static std::vector<Mesh> loadiqm(std::string_view filepath);
	static std::vector<Mesh> loadobj(std::string_view filepath);

	void generatePlane(float size = 1.0f);
	void generateCube(float size = 1.0f);
	void generateSphere(float radius = 1.0f, int segments = 16, int ring = 16);

	struct Vertex
	{
		V3 pos;
		V3 color = { 1.0f,1.0f,1.0f};
		V2 tex;
		V3 normal;
		V3 tangent;
		
		uchar boneIndices[4];
		V4 weights = V4::zero();
	};

protected:
	friend class Model;

	void computeTangents();

	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
};
