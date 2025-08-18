#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include "Common.h"
#include "Rendering/Renderer.h"
#include "Rendering/VisualComponent.h"
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

		Model rabbitModel;
		rabbitModel.load(&renderer, "models/stanford_bunny.obj");	
		Texture bunnyTexture;
		bunnyTexture.load(&renderer, "textures/stanford_bunny.jpg");
		Texture bunnyNormal;
		bunnyNormal.load(&renderer, "textures/stanford_bunny_normal.png", VK_FORMAT_R8G8B8A8_UNORM);
		auto bunnyActor = scene.addActor();
		bunnyActor->addComponent<VisualComponent>()->setModel(&rabbitModel);
		bunnyActor->getComponent< VisualComponent>()->setTexture(&bunnyTexture);
		bunnyActor->getComponent< VisualComponent>()->setNormalMap(&bunnyNormal);
		bunnyActor->getTransformComponent().setTransform(Mtx::translation({ 0.0f, -3.0f, 0.0f }) * Mtx::rotation({ -1.57f, 0.0f, 0.0f }));
		

		/*Model floorModel;
		floorModel.load(&renderer, "models/cube.obj");
		Texture floorTexture;
		floorTexture.load(&renderer, "textures/floor.jpg");
		auto floorActor = scene.addActor();
		floorActor->addComponent<VisualComponent>()->setModel(&floorModel);
		floorActor->getComponent<VisualComponent>()->setTexture(&floorTexture);
		floorActor->getTransformComponent().setTransform(Mtx::rotation({ 1.57f, 0.0f, 0.0f }));*/

		Model planeModel;
		planeModel.generatePlane(&renderer,2);

		auto plane = scene.addActor();
		plane->addComponent<VisualComponent>()->setModel(&planeModel);

		mainLoop();

		rabbitModel.unload();
		//floorModel.unload();

		bunnyTexture.unload();
		bunnyNormal.unload();
		//floorTexture.unload();

		planeModel.unload();

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

			scene.getTransformComponent().setTransform(Mtx::translation({ 0.0f, 2.0f, 0.0f }));
			scene.tick(frameTime);
			renderer.drawFrame(scene, framebufferResized, isPPLightingEnabled);
		}

		std::cout << "Min Frame Time: " << minFrameTime << std::endl;
		renderer.waitUntilDone();
	}

	GLFWwindow* window = nullptr;
	Renderer renderer;

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