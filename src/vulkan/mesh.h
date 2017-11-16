#include "vertexbuffer.h"

#include <tiny_obj_loader.h>
#include <string>

class Device;

class Mesh
{
public:
    bool loadFromObj(Device& device, const std::string& filename, const std::string& materialBaseDir = "");
    void destroy();

    const VertexBuffer& getVertexBuffer() const
    {
        return m_vertexBuffer;
    }

private:
    void createSeparateVertexAttributes(Device& device, const tinyobj::attrib_t& attrib, const std::vector<tinyobj::shape_t>& shapes);
    void createInterleavedVertexAttributes(Device& device, const tinyobj::attrib_t& attrib, const std::vector<tinyobj::shape_t>& shapes);

    VertexBuffer m_vertexBuffer;
};
