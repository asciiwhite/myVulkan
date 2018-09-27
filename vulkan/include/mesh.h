#include "vertexbuffer.h"
#include "graphicspipeline.h"
#include "descriptorset.h"
#include "shader.h"
#include "texture.h"
#include "../utils/glm.h"

#include <tiny_obj_loader.h>
#include <unordered_map>
#include <string>
#include <memory>

class Device;
class RenderPass;
struct Buffer;

class Mesh
{
public:
    bool loadFromObj(Device& device, const std::string& filename);
    void destroy();

    virtual void render(VkCommandBuffer commandBuffer) const;
    virtual bool finalize(VkRenderPass renderPass, VkBuffer cameraUniformBuffer);

    void getBoundingbox(glm::vec3& min, glm::vec3& max) const;
    uint32_t numVertices() const;
    uint32_t numIndices() const;
    uint32_t numShapes() const;
    const std::string& fileName() const;

protected:
    bool hasUniqueVertexAttributes() const;
    void createSeparateVertexAttributes();
    void createInterleavedVertexAttributes();
    virtual Shader selectShaderFromAttributes(bool useTexture);
    void calculateBoundingBox();
    virtual void loadMaterials();
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
    VkPipelineLayout m_pipelineLayout;

    struct MaterialDesc
    {
        Buffer material;
        Shader shader;
        Texture diffuseTexture;
        VkPipeline pipeline;
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

    std::string m_fileName;
};
