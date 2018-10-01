#pragma once

#include "glm.h"

#include "vertexbuffer.h"

#include <string>
#include <vector>


struct ShapeDescription
{
    uint32_t startIndex;
    uint32_t indexCount;
    uint32_t materialId;
};

struct MaterialDescription
{
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 emission;
    std::string textureFilename;
};

struct MeshDescription
{
    std::string name;

    struct BoundingBox
    {
        glm::vec3 min;
        glm::vec3 max;
    } boundingBox;

    struct Geometry
    {
        uint32_t vertexSize = 0;
        uint32_t vertexCount = 0;
        std::vector<VertexBuffer::AttributeDescription> vertexAttribs;
        std::vector<VertexBuffer::InterleavedAttributeDescription> interleavedVertexAttribs;
        std::vector<float> vertices;
        std::vector<uint32_t> indices;
    } geometry;
 
    std::vector<MaterialDescription> materials;
    std::vector<ShapeDescription> shapes;
};
