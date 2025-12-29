#include "SkelAnimation.h"

#include "Skeleton.h"
#include "Importers/Importer_IQM.h"
#include "Engine/Log.h"

std::vector<SkelAnimation> SkelAnimation::load(std::string_view filename)
{
	std::vector<SkelAnimation> result;
	std::ifstream f(filename.data(), std::ios_base::binary);
	if (!f)
		return result;

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
	log(x, x, "Read animations");
	return result;
}

void SkelAnimation::calculateWorldPos(const Skeleton& skeleton)
{
	for (auto& frame : frames)
	{
		skeleton.Visit([&frame](const Skeleton::Bone& skelBone, uint index, uint parentIndex)
		{
			auto& bone = frame.bones[index];
			if (parentIndex != (uint)-1)
			{
				auto& parentBone = frame.bones[parentIndex];
				
				bone.position = parentBone.position + parentBone.rotation.rotate(bone.position);
				bone.rotation = parentBone.rotation * bone.rotation;
				//TODO: scale
			}
		});
	}
	log(x, x, "Baked animations");
}
