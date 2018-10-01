#include "vertexbuffer.h"
#include "graphicspipeline.h"
#include "descriptorset.h"
#include "shader.h"
#include "texture.h"
#include "meshdescription.h"

class Device;
struct Buffer;

class Mesh
{
public:
    bool init(Device& device, const MeshDescription& meshDesc, VkBuffer cameraUniformBuffer, VkRenderPass renderPass);
    void destroy();

    void render(VkCommandBuffer commandBuffer) const;

    uint32_t numVertices() const;
    uint32_t numTriangles() const;

protected:
    void createVertexBuffer(const MeshDescription::Geometry& geometry);

    virtual Shader selectShaderFromAttributes(bool useTexture);
    virtual bool loadMaterials(const std::vector<MaterialDescription>& materials);
    virtual void createDescriptors(VkBuffer cameraUniformBuffer);
    virtual bool createPipelines(VkRenderPass renderPass);

    Device* m_device = nullptr;
    VkSampler m_sampler = VK_NULL_HANDLE;
    VertexBuffer m_vertexBuffer;

    DescriptorSetLayout m_cameraDescriptorSetLayout;
    DescriptorPool m_cameraDescriptorPool;
    DescriptorSet m_cameraUniformDescriptorSet;
    DescriptorSetLayout m_materialDescriptorSetLayout;
    DescriptorPool m_materialDescriptorPool;   
    VkPipelineLayout m_pipelineLayout;

    struct MaterialDesc
    {
        Buffer material;
        Shader shader;
        Texture diffuseTexture;
        VkPipeline pipeline;
        DescriptorSet descriptorSet;
    };
    std::vector<MaterialDesc> m_materials;
    std::vector<ShapeDescription> m_shapes;
};
