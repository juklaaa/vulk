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
#include "Animation/Skeleton.h"
#include "Animation/SkelAnimation.h"
#include "Animation/Mesh.h"
#include "Engine/Scene.h"
#include "Engine/Log.h"

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

class Application
{
public:

	void run()
	{
		Logger::getSingleton().init();
		initWindow();
		renderer.init(window);


		std::string meshPath = "models/tree.iqm";
		meshPath = "models/test1.iqm";
		Skeleton skeleton;
		skeleton.load(meshPath);
		Animations animations;
		SkelAnimation::load(meshPath, animations);
		animations.convertToRootSpace(skeleton);
		auto meshes = Mesh::loadiqm(meshPath);

		//floor
		Material floorMaterial;
		floorMaterial.setColor( 0.0f, 0.7f, 0.2f );
		Model floorModel;
		Mesh planeMesh;
		planeMesh.generatePlane(8);
		floorModel.setMesh(&renderer,&planeMesh);
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
		//tableModel.generateCube(&renderer, 1);
		tableModel.setMesh(&renderer, &meshes[0]);
		auto tableActor = scene.addActor();
		tableActor->addComponent<PhysicsComponent>()->setFlags(PhysicsComponent::Dynamic | PhysicsComponent::Heavy);// ->setAngularVelocity({ 0.0f, 1.0f, 0.0f, 0.001f });
		tableActor->addComponent<BoxColliderComponent>();
		tableActor->addComponent<VisualComponent>()->setModel(&tableModel);
		tableActor->getComponent<VisualComponent>()->setMaterial(&tableMaterial);
		tableActor->getComponent<VisualComponent>()->playAnimation(&animations.animations[0], &animations.initialFrame);
		tableActor->getTransformComponent().setTransform(Mtx::scale({0.5f, 0.5f, 0.5f}) * Mtx::translate({0.0f, 0.0f, 0.0f}));
		
		mainLoop();

		floorModel.unload();
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