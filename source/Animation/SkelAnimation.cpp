#include "SkelAnimation.h"
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

	if (header.num_anims > 0 && header.ofs_anims > 0)
	{
		result.reserve(header.num_anims);
		f.seekg(header.ofs_anims);
		for (uint i = 0; i < header.num_anims; ++i)
		{
			iqmanim anim;
			f.read(reinterpret_cast<char*>(&anim), sizeof(anim));

			//bones.emplace_back(joint.parent >= 0 ? &bones[joint.parent] : nullptr);
		}
	}
	log(x, x, "Read animations");
	return result;
}