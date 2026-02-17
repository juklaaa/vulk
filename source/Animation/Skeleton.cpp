#include "Skeleton.h"
#include "Importers/Importer_IQM.h"
#include "Engine/Log.h"

void Skeleton::load(std::string_view filename)
{
	std::ifstream f(filename.data(), std::ios_base::binary);
	if (!f)
		return;

	iqmheader header;
	f.read(reinterpret_cast<char*>(&header), sizeof(header));

	if (header.num_joints > 0 && header.ofs_joints > 0)
	{
		f.seekg(header.ofs_joints);
		bones.resize(header.num_joints);
		for (uint i = 0; i < header.num_joints; ++i)
		{
			iqmjoint joint;
			f.read(reinterpret_cast<char*>(&joint), sizeof(joint));
			bones[i] = Bone{joint.parent >= 0 ? &bones[joint.parent] : nullptr};
		}
	}
	logLine(x, Verbose, "Read a Skeleton");
}