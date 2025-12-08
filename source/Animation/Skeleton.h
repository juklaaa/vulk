#pragma once

#include "Engine/Math/Math.h"
#include "Common.h"

struct Bone
{
	Bone* parent = nullptr;
};

class Skeleton
{
public:

	void load(std::string_view filename);

protected:
	std::vector<Bone> bones;
};