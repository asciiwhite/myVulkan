#include "mesh.h"
#include "device.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <iostream>
#include <unordered_map>
#include <algorithm>

void Mesh::destroy()
{
    m_vertexBuffer.destroy();
}

bool Mesh::loadFromObj(Device& device, const std::string& filename, const std::string& materialBaseDir)
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string err;

    std::cout << "Loading mesh: " << filename << std::endl;

    bool res = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, filename.c_str(), materialBaseDir.c_str());
    if (!err.empty())
    {
        std::cerr << err << std::endl;
    }
    if (!res)
    {
        std::cerr << "Failed to load " << filename << std::endl;
        return false;
    }

    if ((attrib.normals.size() > 0 && attrib.normals.size() != attrib.vertices.size()) ||
        (attrib.colors.size() > 0 && attrib.colors.size() != attrib.vertices.size()) ||
        (attrib.texcoords.size() > 0 && attrib.texcoords.size() != attrib.vertices.size()))
    {
        createInterleavedVertexAttributes(device, attrib, shapes);
    }
    else
    {
        createSeparateVertexAttributes(device, attrib, shapes);
    }

    return true;
}

void Mesh::createInterleavedVertexAttributes(Device& device, const tinyobj::attrib_t& attrib, const std::vector<tinyobj::shape_t>& shapes)
{
    std::vector<uint32_t> indices;
    uint32_t numIndices = 0;
    std::for_each(shapes.begin(), shapes.end(), [&](const auto& shape) { numIndices += static_cast<uint32_t>(shape.mesh.indices.size()); });
    indices.reserve(numIndices);

    uint32_t vertexSize = 3;
    const uint32_t normalSize = attrib.normals.size() > 0 ? 3 : 0;
    const uint32_t colorSize = attrib.colors.size() == attrib.vertices.size() ? 3 : 0;
    uint32_t texcoordSize = 0;// attrib.texcoords.size() > 0 ? 2 : 0;

    vertexSize += normalSize;
    vertexSize += colorSize;
    vertexSize += texcoordSize;

    std::unordered_map<size_t, uint32_t> uniqueVertices;

    uint32_t vertexOffset = 0;
    std::vector<float> vertexData(vertexSize);

    for (const auto& shape : shapes)
    {
        for (const auto& index : shape.mesh.indices)
        {
            uint32_t currentSize = 0;
            assert(index.vertex_index >= 0);
            memcpy(&vertexData[vertexOffset + currentSize], &attrib.vertices[3 * index.vertex_index], 3 * sizeof(float));
            currentSize += 3;
            if (normalSize > 0)
            {
                assert(index.normal_index >= 0);
                memcpy(&vertexData[vertexOffset + currentSize], &attrib.normals[3 * index.normal_index], 3 * sizeof(float));
                currentSize += normalSize;
            }
            if (colorSize > 0)
            {
                memcpy(&vertexData[vertexOffset + currentSize], &attrib.colors[3 * index.vertex_index], 3 * sizeof(float));
                currentSize += colorSize;
            }
            if (texcoordSize > 0)
            {
                assert(index.texcoord_index >= 0);
                memcpy(&vertexData[vertexOffset + currentSize], &attrib.texcoords[2 * index.texcoord_index], 2 * sizeof(float));
                currentSize += texcoordSize;
            }

            assert(currentSize == vertexSize);
            const auto vertexHash = std::_Hash_seq(reinterpret_cast<const unsigned char*>(&vertexData[vertexOffset]), vertexSize * sizeof(float));
            if (uniqueVertices.count(vertexHash) == 0)
            {
                uniqueVertices[vertexHash] = static_cast<uint32_t>(uniqueVertices.size());
                vertexData.resize(vertexData.size() + vertexSize);
                vertexOffset += vertexSize;
            }

            indices.push_back(uniqueVertices[vertexHash]);
        }
    }

    const auto uniqueVertexCount = static_cast<uint32_t>(uniqueVertices.size());

    uint32_t interleavedOffset = 0;
    std::vector<VertexBuffer::AttributeDescription> vertexDesc(1, { 0, 3, uniqueVertexCount, &vertexData.front() });
    interleavedOffset += 3 * sizeof(float);
    if (normalSize)
    {
        vertexDesc.push_back({ 1, 3, uniqueVertexCount, &vertexData.front(), interleavedOffset });
        interleavedOffset += 3 * sizeof(float);
    }
    if (colorSize)
    {
        vertexDesc.push_back({ 2, 3, uniqueVertexCount, &vertexData.front(), interleavedOffset });
        interleavedOffset += 3 * sizeof(float);
    }
    if (texcoordSize)
    {
        vertexDesc.push_back({ 3, 2, uniqueVertexCount, &vertexData.front(), interleavedOffset });
        interleavedOffset += 2 * sizeof(float);
    }
    assert(interleavedOffset == vertexSize * sizeof(float));

    m_vertexBuffer.createFromInterleavedAttributes(&device, vertexDesc);

    m_vertexBuffer.setIndices(&indices.front(), static_cast<uint32_t>(indices.size()));

    std::cout << "Triangle count:\t\t " << indices.size() / 3 << std::endl;
    std::cout << "Vertex count from file:\t " << attrib.vertices.size() / 3 << std::endl;
    std::cout << "Unique vertex count:\t " << uniqueVertexCount << std::endl;
}

void Mesh::createSeparateVertexAttributes(Device& device, const tinyobj::attrib_t& attrib, const std::vector<tinyobj::shape_t>& shapes)
{
    std::vector<uint32_t> indices;
    uint32_t numIndices = 0;
    std::for_each(shapes.begin(), shapes.end(), [&](const auto& shape) { numIndices += static_cast<uint32_t>(shape.mesh.indices.size()); });
    indices.reserve(numIndices);

    const auto vertexCount =  static_cast<uint32_t>(attrib.vertices.size() / 3);

    std::vector<VertexBuffer::AttributeDescription> vertexDesc(1, { 0, 3, vertexCount, &attrib.vertices.front() });
    if (!attrib.normals.empty())
    {
        assert(attrib.normals.size() == attrib.vertices.size());
        vertexDesc.push_back({ 1, 3, vertexCount, &attrib.normals.front() });
    }
    if (!attrib.colors.empty())
    {
        assert(attrib.colors.size() == attrib.vertices.size());
        vertexDesc.push_back({ 2, 3, vertexCount, &attrib.colors.front() });
    }
    if (!attrib.texcoords.empty())
    {
        assert(attrib.texcoords.size() == attrib.vertices.size());
        vertexDesc.push_back({ 3, 2, vertexCount, &attrib.texcoords.front() });
    }

    m_vertexBuffer.createFromSeparateAttributes(&device, vertexDesc);

    for (const auto& shape : shapes)
    {
        for (const auto& index : shape.mesh.indices)
        {
            assert(index.vertex_index >= 0);
            indices.push_back(static_cast<uint32_t>(index.vertex_index));
        }
    }
    m_vertexBuffer.setIndices(&indices.front(), static_cast<uint32_t>(indices.size()));

    std::cout << "Triangle count:\t " << indices.size() / 3 << std::endl;
    std::cout << "Vertex count:\t " << attrib.vertices.size() / 3 << std::endl;
}
