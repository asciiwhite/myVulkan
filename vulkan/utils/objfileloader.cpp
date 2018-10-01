#include "objfileloader.h"
#include "scopedtimelog.h"
#include "hasher.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <algorithm>
#include <iostream>
#include <unordered_map>
#include <memory>


namespace
{
    using UniqueVertexMap = std::unordered_map<size_t, uint32_t>;

    void calculateBoundingBox(const std::vector<tinyobj::real_t>& vertices, glm::vec3& min, glm::vec3& max)
    {
        assert(!vertices.empty());

        min.x = max.x = vertices[0];
        min.y = max.y = vertices[1];
        min.z = max.z = vertices[2];

        for (uint32_t i = 3; i < vertices.size(); i += 3)
        {
            min.x = std::min(min.x, vertices[i + 0]);
            max.x = std::max(max.x, vertices[i + 0]);
            min.y = std::min(min.y, vertices[i + 1]);
            max.y = std::max(max.y, vertices[i + 1]);
            min.z = std::min(min.z, vertices[i + 2]);
            max.z = std::max(max.z, vertices[i + 2]);
        }
    }

    bool hasUniqueVertexAttributes(const tinyobj::attrib_t& attrib)
    {
        return !(((attrib.normals.empty() != attrib.vertices.empty()) ||
                 (!attrib.texcoords.empty() && attrib.texcoords.size() != attrib.vertices.size())));
    }

    void createSeparateVertexAttributes(const tinyobj::attrib_t& attrib, const std::vector<tinyobj::shape_t>& shapes, MeshDescription& meshDesc)
    {
        ScopedTimeLog log("Creating separate vertex data");

        uint32_t numIndices = 0;
        std::for_each(shapes.begin(), shapes.end(), [&](const tinyobj::shape_t& shape) { numIndices += static_cast<uint32_t>(shape.mesh.indices.size()); });
        meshDesc.geometry.indices.reserve(numIndices);

        meshDesc.shapes.push_back({ 0u, numIndices, 0u });

        for (const auto& shape : shapes)
        {
            for (const auto& index : shape.mesh.indices)
            {
                assert(index.vertex_index >= 0);
                meshDesc.geometry.indices.push_back(static_cast<uint32_t>(index.vertex_index));
            }
        }

        const auto vertexCount = static_cast<uint32_t>(attrib.vertices.size() / 3);

        meshDesc.geometry.vertices.resize(attrib.vertices.size() + attrib.normals.size() + attrib.texcoords.size());
        std::copy(attrib.vertices.begin(), attrib.vertices.end(), meshDesc.geometry.vertices.begin());
        meshDesc.geometry.vertexAttribs.emplace_back(0, 3, vertexCount, meshDesc.geometry.vertices.data());
        if (!attrib.normals.empty())
        {
            const auto offset = attrib.vertices.size();
            std::copy(attrib.normals.begin(), attrib.normals.end(), meshDesc.geometry.vertices.begin() + offset);
            meshDesc.geometry.vertexAttribs.emplace_back(1, 3, vertexCount, meshDesc.geometry.vertices.data() + offset);
        }
        if (!attrib.texcoords.empty())
        {
            const auto offset = attrib.vertices.size() + attrib.normals.size();
            std::copy(attrib.texcoords.begin(), attrib.texcoords.end(), meshDesc.geometry.vertices.end() + offset);
            meshDesc.geometry.vertexAttribs.emplace_back(2, 2, vertexCount, meshDesc.geometry.vertices.data() + offset);
        }
        meshDesc.geometry.vertexSize = 0;
        meshDesc.geometry.vertexCount = vertexCount;

        std::cout << "Triangle count:\t " << meshDesc.geometry.indices.size() / 3 << std::endl;
        std::cout << "Vertex count:\t " << attrib.vertices.size() / 3 << std::endl;
    }

    uint32_t addShape(uint32_t startIndex, int materialId, MeshDescription& meshDesc)
    {
        const auto stopIndex = static_cast<uint32_t>(meshDesc.geometry.indices.size());
        const auto indexCount = stopIndex - startIndex;
        meshDesc.shapes.push_back({ startIndex, indexCount, materialId == -1 ? static_cast<uint32_t>(meshDesc.materials.size()) : static_cast<uint32_t>(materialId) });
        return stopIndex;
    }

    glm::vec3 calculateFaceNormal(const tinyobj::attrib_t& attrib, const tinyobj::index_t idx0, const tinyobj::index_t idx1, const tinyobj::index_t idx2)
    {
        const auto v0 = glm::make_vec3(&attrib.vertices[3 * idx0.vertex_index]);
        const auto v1 = glm::make_vec3(&attrib.vertices[3 * idx1.vertex_index]);
        const auto v2 = glm::make_vec3(&attrib.vertices[3 * idx2.vertex_index]);
        const auto v10 = v1 - v0;
        const auto v20 = v2 - v0;
        return glm::normalize(glm::cross(v10, v20));
    }

    void addVertex(const tinyobj::attrib_t& attrib, const tinyobj::index_t index, UniqueVertexMap& uniqueVertices, const glm::vec3& faceNormal, MeshDescription& meshDesc, uint32_t& vertexOffset)
    {
        assert(index.vertex_index >= 0);

        memcpy(&meshDesc.geometry.vertices[vertexOffset + 0], &attrib.vertices[3 * index.vertex_index], 3 * sizeof(float));
        memcpy(&meshDesc.geometry.vertices[vertexOffset + 3], &faceNormal.x, 3 * sizeof(float));

        uint32_t currentSize = 6;
        if (!attrib.texcoords.empty())
        {
            assert(index.texcoord_index >= 0);
            meshDesc.geometry.vertices[vertexOffset + 6] = attrib.texcoords[2 * index.texcoord_index + 0];
            meshDesc.geometry.vertices[vertexOffset + 7] = 1.0f - attrib.texcoords[2 * index.texcoord_index + 1];
            currentSize += 2;
        }

        assert(meshDesc.geometry.vertexSize == currentSize);
        Hasher hasher;
        hasher.add(reinterpret_cast<const unsigned char*>(&meshDesc.geometry.vertices[vertexOffset]), currentSize * sizeof(float));
        const auto vertexHash = hasher.get();
        if (uniqueVertices.count(vertexHash) == 0)
        {
            uniqueVertices[vertexHash] = static_cast<uint32_t>(uniqueVertices.size());
            meshDesc.geometry.vertices.resize(meshDesc.geometry.vertices.size() + currentSize);
            vertexOffset += currentSize;
        }

        meshDesc.geometry.indices.push_back(uniqueVertices[vertexHash]);
    }

    void mergeShapesByMaterials(MeshDescription& meshDesc)
    {
        // no need for merging if every shape has an unique material
        std::sort(meshDesc.shapes.begin(), meshDesc.shapes.end(), [](const ShapeDescription& a, const ShapeDescription& b) { return a.materialId < b.materialId; });
        auto iter = std::adjacent_find(meshDesc.shapes.begin(), meshDesc.shapes.end(), [](const ShapeDescription& a, const ShapeDescription& b) { return a.materialId == b.materialId; });
        if (iter == meshDesc.shapes.end())
            return;

        ScopedTimeLog log("Merging shapes by material");

        std::vector<uint32_t> mergesIndicies(meshDesc.geometry.indices.size());
        std::vector<ShapeDescription> mergesShapes;

        auto currentMaterialId = meshDesc.shapes.front().materialId;
        mergesShapes.push_back({ 0, 0, currentMaterialId });
        uint32_t mergedShapeIndex = 0;
        for (const auto& shapeDesc : meshDesc.shapes)
        {
            const auto startId = mergesShapes[mergedShapeIndex].startIndex + mergesShapes[mergedShapeIndex].indexCount;

            if (currentMaterialId != shapeDesc.materialId)
            {
                currentMaterialId = shapeDesc.materialId;
                mergesShapes.push_back({ startId, 0, currentMaterialId });
                mergedShapeIndex++;
            }

            std::memcpy(&mergesIndicies[startId], &meshDesc.geometry.indices[shapeDesc.startIndex], shapeDesc.indexCount * sizeof(uint32_t));
            mergesShapes[mergedShapeIndex].indexCount += shapeDesc.indexCount;
        }

        std::cout << "Merged " << meshDesc.shapes.size() << " shapes into " << mergesShapes.size() << " shapes" << std::endl;

        meshDesc.geometry.indices = mergesIndicies;
        meshDesc.shapes = mergesShapes;
    }

    void createInterleavedVertexAttributes(const tinyobj::attrib_t& attrib, const std::vector<tinyobj::shape_t>& shapes, MeshDescription& meshDesc)
    {
        {
            ScopedTimeLog log("Creating interleaved vertex data");

            uint32_t numIndices = 0;
            std::for_each(shapes.begin(), shapes.end(), [&](const tinyobj::shape_t& shape) { numIndices += static_cast<uint32_t>(shape.mesh.indices.size()); });
            meshDesc.geometry.indices.reserve(numIndices);

            std::unordered_map<size_t, uint32_t> uniqueVertices;

            uint32_t shapeStartIndex = 0;
            assert(!shapes[0].mesh.material_ids.empty());
            int currentMaterialId = shapes[0].mesh.material_ids.front();

            meshDesc.geometry.vertexSize = 6 + (attrib.texcoords.empty() ? 0 : 2); // vertex + normals + texcoords
              
            uint32_t vertexOffset = 0;
            meshDesc.geometry.vertices.resize(meshDesc.geometry.vertexSize);
            for (const auto& shape : shapes)
            {
                assert(!shape.mesh.material_ids.empty());

                for (size_t f = 0; f < shape.mesh.indices.size() / 3; f++)
                {
                    if (shape.mesh.material_ids[f] != currentMaterialId)
                    {
                        shapeStartIndex = addShape(shapeStartIndex, currentMaterialId, meshDesc);
                        currentMaterialId = shape.mesh.material_ids[f];
                    }

                    const auto idx0 = shape.mesh.indices[3 * f + 0];
                    const auto idx1 = shape.mesh.indices[3 * f + 1];
                    const auto idx2 = shape.mesh.indices[3 * f + 2];

                    if (attrib.normals.empty())
                    {
                        const auto faceNormal = calculateFaceNormal(attrib, idx0, idx1, idx2);
                        addVertex(attrib, idx0, uniqueVertices, faceNormal, meshDesc, vertexOffset);
                        addVertex(attrib, idx1, uniqueVertices, faceNormal, meshDesc, vertexOffset);
                        addVertex(attrib, idx2, uniqueVertices, faceNormal, meshDesc, vertexOffset);
                    }
                    else
                    {
                        const auto vertexNormal0 = glm::make_vec3(&attrib.normals[3 * idx0.normal_index]);
                        const auto vertexNormal1 = glm::make_vec3(&attrib.normals[3 * idx1.normal_index]);
                        const auto vertexNormal2 = glm::make_vec3(&attrib.normals[3 * idx2.normal_index]);
                        addVertex(attrib, idx0, uniqueVertices, vertexNormal0, meshDesc, vertexOffset);
                        addVertex(attrib, idx1, uniqueVertices, vertexNormal1, meshDesc, vertexOffset);
                        addVertex(attrib, idx2, uniqueVertices, vertexNormal2, meshDesc, vertexOffset);
                    }
                }
            }
            addShape(shapeStartIndex, currentMaterialId, meshDesc);

            meshDesc.geometry.vertexCount = static_cast<uint32_t>(uniqueVertices.size());
            meshDesc.geometry.interleavedVertexAttribs.emplace_back(0u, 3u, 0u);
            meshDesc.geometry.interleavedVertexAttribs.emplace_back(1u, 3u, static_cast<uint32_t>(3u * sizeof(float)));
            if (!attrib.texcoords.empty())
            {
                meshDesc.geometry.interleavedVertexAttribs.emplace_back(2u, 2u, static_cast<uint32_t>(6 * sizeof(float)));
            }

            std::cout << "Triangle count:\t\t " << meshDesc.geometry.indices.size() / 3 << std::endl;
            std::cout << "Vertex count from file:\t " << attrib.vertices.size() / 3 << std::endl;
            std::cout << "Unique vertex count:\t " << meshDesc.geometry.vertexCount << std::endl;
        }
    }

    void convertMaterials(MeshDescription& meshDesc, const std::vector<tinyobj::material_t>& sourceMaterials, const std::string& materialBaseDir)
    {
        meshDesc.materials.resize(sourceMaterials.size() + 1);

        // default material at last
        MaterialDescription defaultMaterial;
        defaultMaterial.ambient = glm::vec3(0.1f);
        defaultMaterial.diffuse = glm::vec3(1.0f);
        defaultMaterial.emission = glm::vec3(0.0f);
        meshDesc.materials[sourceMaterials.size()] = defaultMaterial;

        auto materialDescIter = meshDesc.materials.begin();

        for (auto& sourceMaterial : sourceMaterials)
        {
            auto& materialDesc = *materialDescIter++;

            std::string textureFilename;
            if (!sourceMaterial.diffuse_texname.empty())
            {
                textureFilename = sourceMaterial.diffuse_texname;
            }
            else if (!sourceMaterial.emissive_texname.empty())
            {
                textureFilename = sourceMaterial.emissive_texname;
            }

            if (!textureFilename.empty())
            {
                materialDesc.textureFilename = materialBaseDir + textureFilename;
            }

            materialDesc.ambient = glm::make_vec3(sourceMaterial.ambient);
            materialDesc.diffuse = glm::make_vec3(sourceMaterial.diffuse);
            materialDesc.emission = glm::make_vec3(sourceMaterial.emission);
        }
    }
}

bool ObjFileLoader::read(const std::string& filename, MeshDescription& meshDesc)
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string materialBaseDir;
    std::string err;
    bool result;
    {
        ScopedTimeLog log(("Loading mesh ") + filename);
        materialBaseDir = filename.substr(0, filename.find_last_of("/\\") + 1);
        result = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, filename.c_str(), materialBaseDir.c_str());
    }

    if (!err.empty())
    {
        std::cerr << err << std::endl;
    }
    if (!result)
    {
        std::cerr << "Failed to load " << filename;
        return false;
    }

    if (shapes.empty())
    {
        std::cout << "No shapes to load from" << filename << std::endl;
        return false;
    }

    calculateBoundingBox(attrib.vertices, meshDesc.boundingBox.min, meshDesc.boundingBox.max);

    if (hasUniqueVertexAttributes(attrib))
    {
        
        createSeparateVertexAttributes(attrib, shapes, meshDesc);
    }
    else
    {
        createInterleavedVertexAttributes(attrib, shapes, meshDesc);
    }

    mergeShapesByMaterials(meshDesc);

    convertMaterials(meshDesc, materials, materialBaseDir);
    
    meshDesc.name = filename;

    return true;
}
