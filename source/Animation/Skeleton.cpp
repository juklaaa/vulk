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

	if (header.num_poses > 0 && header.ofs_poses > 0)
	{
		bones.reserve(header.num_poses);
		f.seekg(header.ofs_poses);
		for (uint i = 0; i < header.num_poses; ++i)
		{
			iqmpose joint;
			f.read(reinterpret_cast<char*>(&joint), sizeof(joint));
			bones.emplace_back(joint.parent >= 0 ? &bones[joint.parent] : nullptr);
		}
	}
	log(x, x, "Read a Skeleton");
}