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
#include "Engine/Test/TestObject.h"
#include "Console/Console.h"
#include "Console/ConsoleFunction.h"
#include "Console/GlobalVar.h"

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

GlobalVar<float> gTestVar("testVar", 1.0f);
std::string testConsoleFunc(std::vector<std::string> args)
{
	if (args.size() != 2)
		return "Usage: testConsoleFunc <int value 1> <int value 2>";
	
	int a = std::stoi(args[0]);
	int b = std::stoi(args[1]);
	
	return std::to_string(a + b);
}
ConsoleFunction testConsoleFunc_Wrapper("testConsoleFunc", testConsoleFunc);

class Application
{
public:

	void run()
	{
		console.run();
		
		testTestObject();
		
		Logger::getSingleton().init(&console);
		initWindow();
		renderer.init(window);

		std::string meshPath = "models/tree.iqm";
		//meshPath = "models/clock.iqm";
		meshPath = "models/test.iqm";
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
		tableActor->getTransformComponent().setTransform(Mtx::scale({0.5f, 0.5f, 0.5f}) * Mtx::translate({0.0f, 0.0f, -1.0f}));
		
		mainLoop();

		floorModel.unload();
		tableModel.unload();

		renderer.deinit();
		deinitWindow();
		console.stop();
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
		glfwSetCursorPosCallback(window, cursorPosCallback);
		glfwSetMouseButtonCallback(window, mouseButtonCallback);
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
	
	static void cursorPosCallback(GLFWwindow* window, double xpos, double ypos)
	{
		auto app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
		if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1) == GLFW_PRESS)
			app->sceneRotationZ += 0.01f*(xpos - app->lastCursorPos.x);
		app->lastCursorPos = V2{ (float)xpos, (float)ypos };
	}
	
	static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
	{
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
			
			console.processOnMainThread();
			
			scene.getTransformComponent().setTransform(Mtx::translate({ 0.0f, 2.0f, 0.0f }) * Mtx::rotate({0.0f, 0.0f, sceneRotationZ}));
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
	Console console;

	Scene scene;

	bool framebufferResized = false;
	bool isPPLightingEnabled = true;
	float sceneRotationZ = 0.0f;
	V2 lastCursorPos;
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