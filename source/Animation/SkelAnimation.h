#pragma once

#include "Common.h"
#include "Engine/Math/Math.h"


class SkelAnimation
{
public:

	static std::vector<SkelAnimation> load(std::string_view filename);

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