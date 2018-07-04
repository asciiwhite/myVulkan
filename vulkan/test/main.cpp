#include "basicrenderer.h"
#include "window.h"

#include <GLFW/glfw3.h>
#include <gtest/gtest.h>

class TestRenderer : public BasicRenderer
{
public:
	TestRenderer() {};
	virtual ~TestRenderer() {};

private:
	bool setup() override { return true; };
	void render(uint32_t) override {};
	void shutdown() override {};
	void fillCommandBuffers() override {};
};

static void glfwErrorCallback(int error, const char* description)
{
	std::cout << "glfw error #" << error << " : " << description << "\n";
}

class VulkanBase : public ::testing::Test
{
public:
	void SetUp()
	{
		glfwSetErrorCallback(glfwErrorCallback);
		ASSERT_TRUE(glfwInit());
	}

	void TearDown()
	{
		glfwTerminate();
	}
};

TEST_F(VulkanBase, createWindow)
{
	Window window;
	ASSERT_TRUE(window.init());

	EXPECT_NE(nullptr, window.getWindowHandle());

	window.destroy();	
}

TEST_F(VulkanBase, createBasicRenderer)
{
	Window window;
	ASSERT_TRUE(window.init());

	TestRenderer renderer;
	EXPECT_TRUE(renderer.init(window.getWindowHandle()));

	renderer.destroy();
	window.destroy();
}
