#pragma once

#include "Common.h"

struct Skeleton
{
	void load(std::string_view filename);
	
	struct Bone
	{
		Bone* parent = nullptr;
	};
	
	std::vector<Bone> bones;
	
	template<typename Func>
	void Visit(Func&& func) const
	{
		VisitImpl(func);
	}
	
protected:
	template<typename Func>
	void VisitImpl(Func&& func, const Bone* parent = nullptr, uint parentIndex = (uint)-1) const
	{
		for (uint i = 0u; i < bones.size(); ++i)
		{
			auto& bone = bones[i];
			if (bone.parent == parent)
			{
				func(bone, i, parentIndex);
				VisitImpl(func, &bone, i);
				if (!bone.parent)
					return;
			}
		}
	}
};