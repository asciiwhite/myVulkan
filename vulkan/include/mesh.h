#include "vertexbuffer.h"
#include "graphicspipeline.h"
#include "descriptorset.h"
#include "handles.h"
#include "../utils/glm.h"

#include <tiny_obj_loader.h>
#include <unordered_map>
#include <string>
#include <memory>

class Device;
class RenderPass;

class Mesh
{
public:
    bool loadFromObj(Device& device, const std::string& filename);
    void destroy();

    void render(VkCommandBuffer commandBuffer) const;

    void addCameraUniformBuffer(VkBuffer uniformBuffer);
    bool finalize(const RenderPass& renderPass);
    void getBoundingbox(glm::vec3& min, glm::vec3& max) const;

private:
    bool hasUniqueVertexAttributes() const;
    void createSeparateVertexAttributes();
    void createInterleavedVertexAttributes();
    ShaderHandle selectShaderFromAttributes(bool useTexture);
    void createPipeline();
    void calculateBoundingBox();
    void loadMaterials();
    void createDefaultMaterial();
    void mergeShapesByMaterial();
    void sortShapesByMaterialTransparency();
    bool isTransparentMaterial(uint32_t id) const;

    using UniqueVertexMap = std::unordered_map<size_t, uint32_t>;
    void addVertex(const tinyobj::index_t index, UniqueVertexMap& uniqueVertices, const glm::vec3& faceNormal);
    uint32_t addShape(uint32_t startIndex, int materialId);
    glm::vec3 calculateFaceNormal(const tinyobj::index_t idx0, const tinyobj::index_t idx1, const tinyobj::index_t idx2) const;
    void clearFileData();

    Device* m_device = nullptr;
    VkSampler m_sampler = VK_NULL_HANDLE;
    VertexBuffer m_vertexBuffer;

    glm::vec3 m_minBB;
    glm::vec3 m_maxBB;

    // data from file
    std::string m_materialBaseDir;
    tinyobj::attrib_t m_attrib;
    std::vector<tinyobj::shape_t> m_shapes;
    std::vector<tinyobj::material_t> m_materials;

    // data for backend
    uint32_t m_vertexSize = 0;
    uint32_t m_vertexOffset = 0;
    std::vector<float> m_vertexData;
    std::vector<uint32_t> m_indices;

    DescriptorSetLayout m_cameraDescriptorSetLayout;
    DescriptorPool m_cameraDescriptorPool;
    DescriptorSet m_cameraUniformDescriptorSet;
    DescriptorSetLayout m_materialDescriptorSetLayout;
    DescriptorPool m_materialDescriptorPool;   
    PipelineLayout m_pipelineLayout;

    struct MaterialDesc
    {
        VkBuffer materialUB = VK_NULL_HANDLE;
        VkDeviceMemory materialUBMemory = VK_NULL_HANDLE;
        ShaderHandle shader;
        TextureHandle diffuseTexture;
        PipelineHandle pipeline;
        DescriptorSet descriptorSet;
    };
    std::vector<MaterialDesc> m_materialDescs;

    struct ShapeDesc
    {
        uint32_t startIndex;
        uint32_t indexCount;
        uint32_t materialId;
    };
    std::vector<ShapeDesc> m_shapeDescs;
};
