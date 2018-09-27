#include "mesh.h"

class TextureArrayMesh : public Mesh
{
public:
    void destroy();
    void render(VkCommandBuffer commandBuffer) const override;
    bool finalize(VkRenderPass renderPass, VkBuffer cameraUniformBuffer) override;

private:
    Shader selectShaderFromAttributes(bool useTexture) override;
    void loadMaterials() override;

    std::vector<VkImageView> m_imageViews;
    std::vector<uint32_t> m_materialDescsImageIds;

    DescriptorPool m_mainDescriptorPool;
    DescriptorSetLayout m_texturesDescriptorSetLayout;
    DescriptorSet m_texturesDescriptorSet;
};
