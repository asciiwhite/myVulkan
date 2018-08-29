#include "basicrenderer.h"
#include "window.h"

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

TEST(VulkanBase, createWindow)
{
	Window window;
	ASSERT_TRUE(window.init());

	EXPECT_NE(nullptr, window.getWindowHandle());

	window.destroy();	
}

TEST(VulkanBase, DISABLED_createBasicRenderer)
{
	Window window;
	ASSERT_TRUE(window.init());

	TestRenderer renderer;
	EXPECT_TRUE(renderer.init(window.getWindowHandle()));

	renderer.destroy();
	window.destroy();
}
