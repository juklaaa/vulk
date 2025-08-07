#include "Scene.h"

Scene::~Scene()
{
	for (Actor* actor : actors)
		delete actor;
}

Actor* Scene::addActor()
{
	Actor* newActor = new Actor;
	newActor->scene = this;
	actors.push_back(newActor);
	return newActor;
}

Actor* Scene::addActor(Actor* actor)
{
	actor->scene = this;
	actors.push_back(actor);
	return actor;
}

void Scene::removeActor(Actor* actor)
{
	delete actor;
	std::erase(actors, actor);
}

void Scene::tick(float dt)
{
	Actor::tick(dt);

	for (Actor* actor : actors)
		actor->tick(dt);
}