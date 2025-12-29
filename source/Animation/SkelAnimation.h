#pragma once

#include "Common.h"
#include "Engine/Math/Math.h"

struct Skeleton;

class SkelAnimation
{
public:

	static std::vector<SkelAnimation> load(std::string_view filename);
	
	void calculateWorldPos(const Skeleton& skeleton);

	struct Bone
	{
		V4 position;
		Quat rotation;
		V4 size;
	};

	struct Frame
	{
		std::vector<Bone> bones;
	};

protected:

	std::vector<Frame> frames;
};