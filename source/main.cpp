#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include "Common.h"
#include "Rendering/Renderer.h"
#include "Rendering/VisualComponent.h"
#include "Physics/PhysicsSystem.h"
#include "Physics/PhysicsComponent.h"
#include "Physics/ColliderComponent.h"
#include "Engine/Scene.h"

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

class Application
{
public:

	void run()
	{
		testInverseAlgorithm();
		initWindow();
		renderer.init(window);

		//floor
		Material floorMaterial;
		floorMaterial.setColor( 0.0f, 0.7f, 0.2f );
		Model floorModel;
		floorModel.generatePlane(&renderer,8);
		auto floorActor = scene.addActor();
		floorActor->addComponent<PhysicsComponent>()->setFlags(PhysicsComponent::Heavy);
		floorActor->addComponent<PlaneColliderComponent>()->setEquation({ 0.0f,0.0f,1.0f,1.5f });//floor
		floorActor->addComponent<PlaneColliderComponent>()->setEquation({ 1.0f,0.0f,0.0f,4.0f });//left wall
		floorActor->addComponent<PlaneColliderComponent>()->setEquation({ 1.0f,0.0f,0.0f,-4.0f });//right wall
		floorActor->addComponent<PlaneColliderComponent>()->setEquation({ 0.0f,1.0f,0.0f,4.0f });//back wall
		floorActor->addComponent<PlaneColliderComponent>()->setEquation({ 0.0f,1.0f,0.0f,-4.0f });//front wall
		floorActor->addComponent<PlaneColliderComponent>()->setEquation({ 0.0f,0.0f,1.0f,4.0f });//ceiling
		floorActor->addComponent<VisualComponent>()->setModel(&floorModel);
		floorActor->getComponent<VisualComponent>()->setMaterial(&floorMaterial);
		floorActor->getTransformComponent().setTransform(Mtx::translate({ 0.0f, 0.0f, -1.5f }));

		//table
		Material tableMaterial;
		tableMaterial.setColor(0.9f, 0.9f, 0.2f);
		Model tableModel;
		tableModel.generateCube(&renderer, 1);
		auto tableActor = scene.addActor();
		tableActor->addComponent<PhysicsComponent>()->setFlags(PhysicsComponent::Dynamic | PhysicsComponent::Heavy);// ->setAngularVelocity({ 0.0f, 1.0f, 0.0f, 0.001f });
		tableActor->addComponent<BoxColliderComponent>();
		tableActor->addComponent<VisualComponent>()->setModel(&tableModel);
		tableActor->getComponent<VisualComponent>()->setMaterial(&tableMaterial);
		tableActor->getTransformComponent().setTransform(Mtx::translate({ 0.0f, 0.0f, -1.0f }) * Mtx::scale({ 2.0f, 1.0f, 0.25f }));
		
		//spheres		
		Model sphererModel;
		sphererModel.generateSphere(&renderer, 0.5f, 16, 16);

		const int numSpheres = 5;
		std::vector<Actor*> spheres;
		Material spheresMatirials[numSpheres];
		for (float i = 0; i < numSpheres; i++)
		{
			Material material;
			float c = (i+1) / numSpheres;
			material.setColor(c, 0.0f, c);
			int j = i;
			spheresMatirials[j] = material;

			auto actor = scene.addActor();
			actor->addComponent<VisualComponent>()->setModel(&sphererModel);
			actor->getComponent<VisualComponent>()->setMaterial(&spheresMatirials[j]);
			actor->getTransformComponent().setTransform(Mtx::translate({ i * 1.1f - 3, 0.0f, 1.0f }));
			actor->addComponent<PhysicsComponent>()->setFlags(PhysicsComponent::Dynamic | PhysicsComponent::Gravity)->setVelocity(V4{ 0.001f, 0.0f, 0.0f });
			actor->addComponent<SphereColliderComponent>();
			actor->getComponent<PhysicsComponent>()->setMass(i + numSpheres);
			actor->getComponent<PhysicsComponent>()->setRestitution(0.99f);
			spheres.push_back(actor);
		}

		mainLoop();

		floorModel.unload();
		sphererModel.unload();
		tableModel.unload();

		renderer.deinit();
		deinitWindow();
	}

private:

	void initWindow()
	{
		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
		glfwSetWindowUserPointer(window, this);
		glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
		glfwSetKeyCallback(window, keyCallback);
	}

	static void framebufferResizeCallback(GLFWwindow* window, int width, int height)
	{
		auto app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
		app->framebufferResized = true;
	}

	static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
	{
		auto app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
		if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
		{
			app->isPPLightingEnabled = !app->isPPLightingEnabled;
		}
	}

	void deinitWindow()
	{
		glfwDestroyWindow(window);
		glfwTerminate();
	}

	void mainLoop()
	{
		auto tlast = std::chrono::high_resolution_clock::now();
		float minFrameTime = std::numeric_limits<float>::max();
		while (!glfwWindowShouldClose(window))
		{
			auto tnow = std::chrono::high_resolution_clock::now();
			std::chrono::duration<float, std::milli> frameTimeDur = tnow - tlast;
			tlast = tnow;
			float frameTime = frameTimeDur.count();
			minFrameTime = frameTime < minFrameTime ? frameTime : minFrameTime;

			glfwPollEvents();

			scene.getTransformComponent().setTransform(Mtx::translate({ 0.0f, 2.0f, 0.0f }));
			physics.update(scene, frameTime);
			scene.tick(frameTime);
			renderer.drawFrame(scene, framebufferResized, isPPLightingEnabled);
		}

		std::cout << "Min Frame Time: " << minFrameTime << std::endl;
		renderer.waitUntilDone();
	}

	GLFWwindow* window = nullptr;
	Renderer renderer;
	PhysicsSystem physics;

	Scene scene;

	bool framebufferResized = false;
	bool isPPLightingEnabled = true;
};

int main()
{
	Application app;

	try
	{
		app.run();
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}