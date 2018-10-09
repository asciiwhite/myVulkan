#include "vertexbuffer.h"
#include "graphicspipeline.h"
#include "descriptorset.h"
#include "shader.h"
#include "texture.h"
#include "meshdescription.h"

class Device;
struct Buffer;

class Mesh : public DeviceRef
{
public:
    Mesh(Device& device);
    ~Mesh();

    bool init(const MeshDescription& meshDesc, VkBuffer cameraUniformBuffer, VkRenderPass renderPass);
    void render(VkCommandBuffer commandBuffer) const;

    uint32_t numVertices() const;
    uint32_t numTriangles() const;
    uint32_t numShapes() const;

protected:
    void createVertexBuffer(const MeshDescription::Geometry& geometry);

    Shader selectShaderFromAttributes(bool useTexture);
    bool loadMaterials(const std::vector<MaterialDescription>& materials);
    void createDescriptors(VkBuffer cameraUniformBuffer);
    bool createPipelines(VkRenderPass renderPass);

    VkSampler m_sampler = VK_NULL_HANDLE;
    VertexBuffer m_vertexBuffer;

    VkDescriptorSetLayout m_cameraDescriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool m_cameraDescriptorPool = VK_NULL_HANDLE;
    DescriptorSet m_cameraUniformDescriptorSet;
    VkDescriptorSetLayout m_materialDescriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool m_materialDescriptorPool = VK_NULL_HANDLE;
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;

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
