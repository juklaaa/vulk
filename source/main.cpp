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
		Logger::getSingleton().init(&console);
		
		testTestObject();
		
		initWindow();
		renderer.init(window);

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

		//cat
		std::string meshPath = "models/cat.iqm";
		Skeleton skeleton;
		skeleton.load(meshPath);
		Animations animations;
		SkelAnimation::load(meshPath, animations);
		animations.convertToRootSpace(skeleton);
		auto meshes = Mesh::loadiqm(meshPath);
		Texture catTexture;
		catTexture.load(&renderer, "textures/cat.png");
		Material catMaterial;
		catMaterial.setTexture(&catTexture);
		Model catModel;
		catModel.setMesh(&renderer, &meshes[0]);
		catActor = scene.addActor();
		catActor->addComponent<VisualComponent>()->setModel(&catModel);
		catActor->getComponent<VisualComponent>()->setMaterial(&catMaterial);
		catActor->getComponent<VisualComponent>()->playAnimation(&animations.animations[0], &animations.initialFrame);
		catActor->getTransformComponent().setTransform(Mtx::scale({ 0.25f, 0.25f, 0.25f }) * Mtx::translate({ 0.0f, 0.0f, -1.25f }));
		
		mainLoop();

		floorModel.unload();
		catModel.unload();
		catTexture.unload();

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

		if (key == GLFW_KEY_W && action == GLFW_PRESS)
		{
			app->playerInputForward = 0.1f;
		}
		if (key == GLFW_KEY_W && action == GLFW_RELEASE)
		{
			app->playerInputForward = 0.0f;
		}

		if (key == GLFW_KEY_S && action == GLFW_PRESS)
		{
			app->playerInputForward = -0.1f;
		}

		if (key == GLFW_KEY_S && action == GLFW_RELEASE)
		{
			app->playerInputForward = 0.0f;
		}

		if (key == GLFW_KEY_A && action == GLFW_PRESS)
		{
			app->playerInputRight = -0.1f;
		}

		if (key == GLFW_KEY_A && action == GLFW_RELEASE)
		{
			app->playerInputRight = 0.0f;
		}

		if (key == GLFW_KEY_D && action == GLFW_PRESS)
		{
			app->playerInputRight = 0.1f;
		}

		if (key == GLFW_KEY_D && action == GLFW_RELEASE)
		{
			app->playerInputRight = 0.0f;
		}
	}
	
	static void cursorPosCallback(GLFWwindow* window, double xpos, double ypos)
	{
		auto app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
		if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1) == GLFW_PRESS)
			app->playerRotationZ = 0.001f*(xpos - app->lastCursorPos.x);
		else
			app->playerRotationZ = 0.0f;
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
			
			if (catActor)
			{
				auto catTransform = catActor->getTransformComponent().getTransform();
				
				if (playerInputForward != 0.0f || playerInputRight != 0.0f)
				{
					auto catPos = catTransform.getPosition();
					if (playerInputForward < 0.0f)
						catTransform = cameraTransform.getRotation() * Mtx::rotate({0.0f, 0.0f, PI});
					else
						catTransform = cameraTransform.getRotation() * Mtx::rotate({0.0f, 0.0f, glm::sign(playerInputRight) * PI/2});
					catTransform = Mtx::scale({0.25f, 0.25f, 0.25f}) * catTransform;
					catTransform = catTransform * Mtx::translate(catPos);
				}
				auto playerInput = playerInputForward != 0.0f ? fabs(playerInputForward) : fabs(playerInputRight);
				catTransform = catTransform * Mtx::translate(catTransform.getForward() * 0.25f * frameTime * playerInput);
				catActor->getTransformComponent().setTransform(catTransform);
				
				V4 playerPos = catTransform.getPosition();
				
				cameraTransform = cameraTransform * Mtx::rotate(V4{0.0f, 0.0f, frameTime * playerRotationZ});
				renderer.cameraLookAt = glm::vec3(playerPos.x, playerPos.y, playerPos.z);
				auto cameraWorldTransform = (cameraTransform * Mtx::translate(catTransform.getPosition())).getPosition();
				renderer.cameraPos = glm::vec3(cameraWorldTransform.x, cameraWorldTransform.y, cameraWorldTransform.z);
			}
			
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
	float playerRotationZ = 0.0f;
	float playerInputForward = 0.0f;
	float playerInputRight = 0.0f;
	V2 lastCursorPos;
	Actor* catActor = nullptr;
	Mtx cameraTransform = Mtx::translate({4.0f, 0.0f, 4.0f});
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