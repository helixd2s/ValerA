#pragma once
#include "./Config.hpp"
#include "./Driver.hpp"
#include "./VertexSet.hpp"

namespace vlr {

#pragma pack(push, 1)
    struct MeshIDFlags {
        uint32_t ID : 24;
        uint32_t hasTransform : 1, hasNormal : 1, hasTexcoord : 1, hasTangent : 1, reserved : 4;
    };
#pragma pack(pop)

    struct GeometryDesc {
        glm::mat3x4 transform = glm::mat3x4(1.f);
        uint32_t firstVertex = 0u;
        uint32_t primitiveCount = 0u;
        uint32_t material = 0u;
        
        // 
        union {
            MeshIDFlags mesh_flags;
            uint32_t mesh_flags_u32 = 0u;
        };

        // We solved to re-port into... 
        uint32_t vertexAttribute = 0u, indexBufferView = ~0u, indexType = VK_INDEX_TYPE_NONE_KHR, reserved = 0u;
        //VkIndexType indexType = VK_INDEX_TYPE_NONE_KHR;
    };

    class Geometry : public std::enable_shared_from_this<Geometry> { protected: friend GeometrySet; friend Acceleration; friend Rasterization;
        vkt::uni_ptr<VertexSet> vertexSet = {};
        vkt::uni_arg<GeometryDesc> desc = {};

    public: 
        Geometry() { this->constructor(); };
        Geometry(vkt::uni_ptr<VertexSet> vertexSet, vkt::uni_arg<GeometryDesc> desc = {}) { this->constructor(vertexSet, desc); };
        ~Geometry() {};

        virtual void constructor() {};
        virtual void constructor(vkt::uni_ptr<VertexSet> vertexSet, vkt::uni_arg<GeometryDesc> desc = {}) {
            this->vertexSet = vertexSet, this->desc = desc;
            auto buffer = this->vertexSet->getAttributeBuffer_T(this->desc->vertexAttribute);
            this->desc->primitiveCount = std::min(this->desc->primitiveCount, uint32_t(buffer.range() / (buffer.stride() * 3ull))); // Make Bit Safer
        };
        virtual void setIndexBuffer(uint32_t indexBufferView = ~0u, VkIndexType indexType = VK_INDEX_TYPE_NONE_KHR) {
            this->desc->indexBufferView = indexBufferView, this->desc->indexType = indexType;
        };
        virtual void setVertexBuffer(uint32_t vertexAttribute = 0u){
            this->desc->vertexAttribute = vertexAttribute;
        };
    };

};
