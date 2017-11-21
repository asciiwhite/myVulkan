#include "vertexbuffer.h"
#include "pipeline.h"
#include "descriptorset.h"
#include "..\utils\glm.h"

#include <tiny_obj_loader.h>
#include <string>
#include <memory>

class Device;
class Shader;
class Texture;
class RenderPass;

class Mesh
{
public:
    bool loadFromObj(Device& device, const std::string& filename);
    void destroy();

    void render(VkCommandBuffer commandBuffer) const;

    void addUniformBuffer(VkShaderStageFlags shaderStages, VkBuffer uniformBuffer);
    bool finalize(const RenderPass& renderPass);
    void getBoundingbox(glm::vec3& min, glm::vec3& max) const;

private:
    void createSeparateVertexAttributes(const tinyobj::attrib_t& attrib, const std::vector<tinyobj::shape_t>& shapes, const std::vector<tinyobj::material_t>& materials);
    void createInterleavedVertexAttributes(const tinyobj::attrib_t& attrib, const std::vector<tinyobj::shape_t>& shapes, const std::vector<tinyobj::material_t>& materials);
    std::shared_ptr<Shader> selectShaderFromAttributes(const tinyobj::attrib_t& attrib);
    void createPipeline();
    void calculateBoundingBox(const std::vector<float>& vertices, uint32_t stride);
    void loadMaterials(const std::vector<tinyobj::shape_t>& shapes, const std::vector<tinyobj::material_t>& materials);

    Device* m_device = nullptr;
    std::shared_ptr<Shader> m_shader;
    std::shared_ptr<Texture> m_texture;
    VkSampler m_sampler = VK_NULL_HANDLE;
    VertexBuffer m_vertexBuffer;
    DescriptorSet m_descriptorSet;
    PipelineLayout m_pipelineLayout;
    Pipeline m_pipeline;

    glm::vec3 m_minBB;
    glm::vec3 m_maxBB;

    std::string m_materialBaseDir;
};
