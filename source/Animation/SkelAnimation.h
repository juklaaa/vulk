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

	uint getName() const { return name; }
	float getFramerate() const { return framerate; }
	uint getNumFrames() const { return frames.size(); }
	const Frame& getFrame(uint frameIndex) const { return frames[frameIndex]; }
	
protected:

	uint name = 0u;
	std::vector<Frame> frames;
	float framerate = 1.0f;
};

struct Animations
{
	std::vector<SkelAnimation> animations;
	SkelAnimation::Frame initialFrame;
	void convertToRootSpace(const Skeleton& skeleton);
};