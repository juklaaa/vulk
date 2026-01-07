#include "SkelAnimation.h"

#include "Skeleton.h"
#include "Importers/Importer_IQM.h"
#include "Engine/Log.h"

void SkelAnimation::load(std::string_view filename, Animations& animations)
{
	std::vector<SkelAnimation>& result = animations.animations;
	std::ifstream f(filename.data(), std::ios_base::binary);
	if (!f)
		return;

	iqmheader header;
	f.read(reinterpret_cast<char*>(&header), sizeof(header));

	std::vector<uint> boneMasks;
	struct BoneFloats
	{
		float data[10];
		float dataScale[10];
	};
	std::vector<BoneFloats> boneDefaults;
	if (header.num_poses > 0 && header.ofs_poses > 0)
	{
		boneMasks.reserve(header.num_poses);
		boneDefaults.reserve(header.num_poses);
		f.seekg(header.ofs_poses);
		for (uint i = 0; i < header.num_poses; ++i)
		{
			iqmpose pose;
			f.read(reinterpret_cast<char*>(&pose), sizeof(pose));
			boneMasks.push_back(pose.channelmask);
			BoneFloats floats;
			memcpy(floats.data, pose.channeloffset, sizeof(pose.channeloffset));
			memcpy(floats.dataScale, pose.channelscale, sizeof(pose.channelscale));
			boneDefaults.push_back(floats);
		}
	}

	if (header.num_anims > 0 && header.ofs_anims > 0)
	{
		result.reserve(header.num_anims);
		f.seekg(header.ofs_anims);
		for (uint i = 0; i < header.num_anims; ++i)
		{
			iqmanim anim;
			f.read(reinterpret_cast<char*>(&anim), sizeof(anim));
			
			SkelAnimation skelAnim;
			skelAnim.name = anim.name;
			skelAnim.framerate = anim.framerate;
			
			f.seekg(header.ofs_frames/* + anim.first_frame * frame_size */);
			for (int j = 0; j < anim.num_frames; ++j)
			{
				Frame frame;
				for (int boneIndex = 0; boneIndex < boneMasks.size(); ++boneIndex)
				{
					uint mask = boneMasks[boneIndex];
					int numChannels = std::popcount(mask);
					ushort data[10];
					memset(data, 0, sizeof(data));
					for (int k = 0; k < 10; ++k)
					{
						if (mask & (1 << k))
						{
							f.read(reinterpret_cast<char*>(&data[k]), sizeof(ushort));
						}
					}
					float dataFloat[10];
					memset(dataFloat, 0, sizeof(dataFloat));
					for (int k = 0; k < 10; ++k)
					{
						dataFloat[k] = boneDefaults[boneIndex].data[k];
						if (boneMasks[boneIndex] & (1 << k))
						{
							dataFloat[k] += (float)data[k] * boneDefaults[boneIndex].dataScale[k];
						}
					}
					auto& d = dataFloat;
					frame.bones.emplace_back(V4{ d[0], d[1], d[2] }, Quat{ d[3], d[4], d[5], d[6] }, V4{ d[7], d[8], d[9] });
				}
				skelAnim.frames.push_back(std::move(frame));
			}
			result.push_back(std::move(skelAnim));
		}
	}
	
	if (header.num_joints > 0 && header.ofs_joints > 0)
	{
		f.seekg(header.ofs_joints);
		for (uint i = 0; i < header.num_joints; ++i)
		{
			iqmjoint joint;
			f.read(reinterpret_cast<char*>(&joint), sizeof(joint));
			animations.initialFrame.bones.emplace_back
			(
				V4{joint.translate[0],joint.translate[1],joint.translate[2]},
				Quat{joint.rotate[0],joint.rotate[1],joint.rotate[2], joint.rotate[3]},
				V4{joint.scale[0],joint.scale[1],joint.scale[2]}
			);
		}
	}
}


void SkelAnimation::convertToRootSpace(const Skeleton& skeleton)
{
	for (auto& frame : frames)
		frame.convertToRootSpace(skeleton);
}
		
void SkelAnimation::Frame::convertToRootSpace(const Skeleton& skeleton)
{
	skeleton.Visit([this](const Skeleton::Bone& skelBone, uint index, uint parentIndex)
	{
		auto& bone = bones[index];
		if (parentIndex != (uint)-1)
		{
			auto& parentBone = bones[parentIndex];
			
			bone.position = parentBone.position + parentBone.rotation.rotate(bone.position);
			bone.rotation = parentBone.rotation * bone.rotation;
			//TODO: scale
		}
	});
}

void Animations::convertToRootSpace(const Skeleton& skeleton)
{
	for (auto& animation : animations)
		animation.convertToRootSpace(skeleton);
	initialFrame.convertToRootSpace(skeleton);
}
