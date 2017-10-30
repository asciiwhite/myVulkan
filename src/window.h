struct GLFWwindow;
class BasicRenderer;

class Window
{
public:
    bool init();
    void destroy();

    void show();
    GLFWwindow* getWindowHandle() const;

    void run(BasicRenderer& renderer);

private:
    void resize(int width, int height);

    BasicRenderer* m_renderer = nullptr;
    GLFWwindow* m_window = nullptr;
    bool m_pause = false;
};
