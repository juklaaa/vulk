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
		memset(playerInputKeys, 0, sizeof(playerInputKeys));

		//floor
		Material floorMaterial;
		floorMaterial.setColor( 0.0f, 0.7f, 0.2f );
		Model floorModel;
		Mesh planeMesh;
		planeMesh.generatePlane(32.0f);
		floorModel.setMesh(&renderer,&planeMesh);
		auto floorActor = scene.addActor();
		floorActor->addComponent<PhysicsComponent>()->setFlags(PhysicsComponent::Heavy);
		floorActor->addComponent<PlaneColliderComponent>()->setEquation({ 0.0f,0.0f,1.0f,0.0f });//floor
		floorActor->addComponent<PlaneColliderComponent>()->setEquation({ 1.0f,0.0f,0.0f,16.0f });//left wall
		floorActor->addComponent<PlaneColliderComponent>()->setEquation({ 1.0f,0.0f,0.0f,-16.0f });//right wall
		floorActor->addComponent<PlaneColliderComponent>()->setEquation({ 0.0f,1.0f,0.0f,16.0f });//back wall
		floorActor->addComponent<PlaneColliderComponent>()->setEquation({ 0.0f,1.0f,0.0f,-16.0f });//front wall
		floorActor->addComponent<PlaneColliderComponent>()->setEquation({ 0.0f,0.0f,1.0f,16.0f });//ceiling
		floorActor->addComponent<VisualComponent>()->setModel(&floorModel);
		floorActor->getComponent<VisualComponent>()->setMaterial(&floorMaterial);

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
		Mesh boxMesh;
		boxMesh.generateCube(1.0f);
		//catModel.setMesh(&renderer, &boxMesh);
		catActor = scene.addActor();
		catActor->addComponent<VisualComponent>()->setModel(&catModel)->setMaterial(&catMaterial);
		
		catActor->getComponent<VisualComponent>()->playAnimation(&animations.animations[0], &animations.initialFrame);
		catActor->getTransformComponent().setTransform(Mtx::scale(V4{0.25f, 0.25f, 0.25f}));
		catActor->addComponent<PhysicsComponent>()->setFlags(PhysicsComponent::Heavy);
		catActor->addComponent<SphereColliderComponent>();
		
		std::default_random_engine random_engine(1);
		
		// balls
		const int numBalls = 12;
		Mesh ballMesh;
		ballMesh.generateSphere(0.5f);
		Model ballModel;
		ballModel.setMesh(&renderer, &ballMesh);
		Material ballMaterial;
		ballMaterial.setColor(1.0f, 0.8f, 0.05f);
		for (int i = 0; i < numBalls; i++)
		{
			auto ballActor = scene.addActor();
			ballActor->addComponent<VisualComponent>()->setModel(&ballModel)->setMaterial(&ballMaterial);
			ballActor->addComponent<PhysicsComponent>()->setMass(1.0f)->setFlags(PhysicsComponent::Dynamic | PhysicsComponent::Gravity)->setRestitution(0.0f);
			ballActor->addComponent<SphereColliderComponent>();
			std::uniform_real_distribution d(-10.0f, 10.0f);
			ballActor->getTransformComponent().setTransform(Mtx::translate({d(random_engine), d(random_engine), 4.0f}));
		}
		
		mainLoop();

		floorModel.unload();
		catModel.unload();
		catTexture.unload();
		ballModel.unload();

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
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	}

	static void framebufferResizeCallback(GLFWwindow* window, int width, int height)
	{
		auto app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
		app->framebufferResized = true;
	}

	static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
	{
		auto app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
		app->playerInputKeys[key] = (action == GLFW_PRESS || action == GLFW_REPEAT);
	}
	
	static void cursorPosCallback(GLFWwindow* window, double xpos, double ypos)
	{
		auto app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
		V2 mpos{(float)xpos, (float)ypos};
		app->playerInputMouseDelta = mpos - app->lastCursorPos;
		app->lastCursorPos = mpos;
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
		
		while (!glfwWindowShouldClose(window))
		{
			auto tnow = std::chrono::high_resolution_clock::now();
			std::chrono::duration<float, std::milli> frameTimeDur = tnow - tlast;
			tlast = tnow;
			float frameTime = frameTimeDur.count();

			glfwPollEvents();
			
			console.processOnMainThread();
			
			V2 inputMotion{0.0f, 0.0f};
			inputMotion += V2(1.0f, 0.0f) * playerInputKeys[GLFW_KEY_D];
			inputMotion += V2(-1.0f, 0.0f) * playerInputKeys[GLFW_KEY_A];
			inputMotion += V2(0.0f, 1.0f) * playerInputKeys[GLFW_KEY_W];
			inputMotion += V2(0.0f, -1.0f) * playerInputKeys[GLFW_KEY_S];
			inputMotion = inputMotion.normalize();
			
			if (catActor)
			{
				auto catTransform = catActor->getTransformComponent().getTransform();
				const float catSpeed = 0.005f;
				const float catRotateSpeed = 0.01f;
				
				if (inputMotion.length2() > 0.0f)
				{
					V4 cameraForward2D = V4{cos(cameraAngles.x), sin(cameraAngles.x), 0.0f} * -1.0f;
					V4 catForward2D = catTransform.getForward().multiply({1.0f, 1.0f, 0.0f}).normalize();
					Quat inputQuat = Quat::from2Vecs(V4{0.0f, 1.0f, 0.0f}, V4{inputMotion});
					if (inputMotion.dot({0.0f, 1.0f}) == -1.0f)
						inputQuat = Quat(0.0f, 0.0f, 1.0f, 0.0f);
					V4 catDesiredForward2D = inputQuat.rotate(cameraForward2D).normalize();
					
					float dot = catForward2D.dot(catDesiredForward2D);
					if (dot < 0.999f)
					{
						float catRotateDir = -glm::sign(catForward2D.cross(catDesiredForward2D).z);
						V4 catPos = catTransform.getPosition();
						catTransform.rows[3] = {0.0f, 0.0f, 0.0f, 1.0f};
						catTransform = catTransform * Mtx::rotate({0.0f, 0.0f, frameTime * catRotateDir * catRotateSpeed});
						catTransform = catTransform * Mtx::translate(catPos);
					}
					catTransform = catTransform * Mtx::translate(catTransform.getForward() * frameTime * catSpeed);
					catActor->getTransformComponent().setTransform(catTransform);
				}
				catActor->getComponent<VisualComponent>()->setAnimationSpeed(inputMotion.length());
				
				cameraAngles += V2{-playerInputMouseDelta.x, playerInputMouseDelta.y} * 0.001f;
				cameraAngles.y = std::clamp(cameraAngles.y, 0.0f, 0.9f*PI/2);
				V4 cameraPosition = V4{cos(cameraAngles.x)*cos(cameraAngles.y), sin(cameraAngles.x)*cos(cameraAngles.y), sin(cameraAngles.y)} * cameraRadius;
				V4 playerPos = catTransform.getPosition();
				cameraPosition += playerPos;
				
				renderer.cameraLookAt = glm::vec3(playerPos.x, playerPos.y, playerPos.z);
				renderer.cameraPos = glm::vec3(cameraPosition.x, cameraPosition.y, cameraPosition.z);
			}
			
			physics.update(scene, frameTime);
			scene.tick(frameTime);
			renderer.drawFrame(scene, framebufferResized, true);
			playerInputMouseDelta = V2{0.0f, 0.0f};
		}

		renderer.waitUntilDone();
	}

	GLFWwindow* window = nullptr;
	Renderer renderer;
	PhysicsSystem physics;
	Console console;

	Scene scene;

	bool framebufferResized = false;
	
	V2 lastCursorPos;
	bool playerInputKeys[GLFW_KEY_LAST];
	V2 playerInputMouseDelta = V2{0.0f, 0.0f};
	
	Actor* catActor = nullptr;
	V2 cameraAngles = V2{0.0f, 0.0f};
	float cameraRadius = 5.0f;
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