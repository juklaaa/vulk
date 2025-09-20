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
		initWindow();
		renderer.init(window);

		//floor
		Material floorMaterial;
		floorMaterial.setColor( 0.0f, 0.7f, 0.2f );
		Model floorModel;
		floorModel.generatePlane(&renderer,8);
		auto floorActor = scene.addActor();
		//floor coliders
		floorActor->getTransformComponent().setTransform(Mtx::translate({ 0.0f, 0.0f, 0.0f }));
		floorActor->addComponent<PhysicsComponent>()->setMass(9999.0f);
		floorActor->addComponent<PlaneColliderComponent>()->setEquation({ 0.0f,0.0f,1.0f,1.5f });//floor
		floorActor->addComponent<PlaneColliderComponent>()->setEquation({ 1.0f,0.0f,0.0f,4.0f });//left wall
		floorActor->addComponent<PlaneColliderComponent>()->setEquation({ 1.0f,0.0f,0.0f,-4.0f });//right wall
		floorActor->addComponent<PlaneColliderComponent>()->setEquation({ 0.0f,1.0f,0.0f,4.0f });//back wall
		floorActor->addComponent<PlaneColliderComponent>()->setEquation({ 0.0f,1.0f,0.0f,-4.0f });//front wall
		floorActor->addComponent<PlaneColliderComponent>()->setEquation({ 0.0f,0.0f,1.0f,5.0f });//ceiling

		//floor but only model 
		auto floorActor2 = scene.addActor();
		floorActor2->addComponent<VisualComponent>()->setModel(&floorModel);
		floorActor2->getComponent< VisualComponent>()->setMaterial(&floorMaterial);
		floorActor2->getTransformComponent().setTransform(Mtx::translate({ 0.0f, 0.0f, -1.5f }));

		//spheres
		Material sphereMaterial;
		sphereMaterial.setColor(1.0f, 0.0f, 1.0f);
		Model sphererModel;
		sphererModel.generateSphere(&renderer, 0.5f, 16, 16);

		auto sphereActor = scene.addActor();
		sphereActor->addComponent<VisualComponent>()->setModel(&sphererModel);
		sphereActor->getComponent<VisualComponent>()->setMaterial(&sphereMaterial);
		sphereActor->getTransformComponent().setTransform(Mtx::translate({0.0f, 0.0f, 2.0f }));
		sphereActor->addComponent<PhysicsComponent>();
		sphereActor->addComponent<SphereColliderComponent>();

		auto sphereActor2 = scene.addActor();
		sphereActor2->addComponent<VisualComponent>()->setModel(&sphererModel);
		sphereActor2->getComponent<VisualComponent>()->setMaterial(&sphereMaterial);
		sphereActor2->getTransformComponent().setTransform(Mtx::translate({-3.0f, 0.0f, 1.0f }));
		sphereActor2->addComponent<PhysicsComponent>() ->setVelocity(V4{ 0.001f, 0.0f, 0.0f });
		sphereActor2->addComponent<SphereColliderComponent>();
		sphereActor2->getComponent<PhysicsComponent>()->setRestitution(0.99f);
		
		mainLoop();

		floorModel.unload();
		sphererModel.unload();

		


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