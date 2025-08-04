#pragma once

#include "Actor.h"

class Scene : public Actor
{
public:

	~Scene();

	Actor* addActor();
	Actor* addActor(Actor* actor);
	void removeActor(Actor* actor);

	const std::vector<Actor*>& getActors() const { return actors; }

	template<typename Func>
	void forAllActors(const Func& func)
	{
		for (Actor* actor : actors)
		{
			func(actor);
			if (Scene* scene = dynamic_cast<Scene*>(actor))
				scene->forAllActors(func);
		}
	}

	virtual void tick(float dt) override;

protected:

private:
	std::vector<Actor*> actors;
};