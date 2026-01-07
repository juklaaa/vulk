#pragma once

#include "Common.h"
#include "Engine/Math/Math.h"

struct Skeleton;
struct Animations;

class SkelAnimation
{
public:

	static void load(std::string_view filename, Animations& animations);
	
	void convertToRootSpace(const Skeleton& skeleton);

	struct Bone
	{
		V4 position;
		Quat rotation;
		V4 size;
	};

	struct Frame
	{
		void convertToRootSpace(const Skeleton& skeleton);
		std::vector<Bone> bones;
	};

protected:

	std::vector<Frame> frames;
};

struct Animations
{
	std::vector<SkelAnimation> animations;
	SkelAnimation::Frame initialFrame;
	void convertToRootSpace(const Skeleton& skeleton);
};