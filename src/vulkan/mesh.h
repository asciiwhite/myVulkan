#include "vertexbuffer.h"
#include "pipeline.h"
#include "descriptorset.h"

#include <tiny_obj_loader.h>
#include <string>
#include <memory>

class Device;
class Shader;
class RenderPass;

class Mesh
{
public:
    bool loadFromObj(Device& device, const std::string& filename);
    void destroy();

    void render(VkCommandBuffer commandBuffer) const;

    void addUniformBuffer(VkShaderStageFlags shaderStages, VkBuffer uniformBuffer);
    bool finalize(const RenderPass& renderPass);

private:
    void createSeparateVertexAttributes(const tinyobj::attrib_t& attrib, const std::vector<tinyobj::shape_t>& shapes);
    void createInterleavedVertexAttributes(const tinyobj::attrib_t& attrib, const std::vector<tinyobj::shape_t>& shapes);
    std::shared_ptr<Shader> selectShaderFromAttributes(const tinyobj::attrib_t& attrib);
    void createPipeline();

    Device* m_device = nullptr;
    std::shared_ptr<Shader> m_shader;
    VertexBuffer m_vertexBuffer;
    DescriptorSet m_descriptorSet;
    PipelineLayout m_pipelineLayout;
    Pipeline m_pipeline;
};
