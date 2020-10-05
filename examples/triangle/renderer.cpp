#pragma once

// 
#include "./stdafx.h"
//#include "renderdoc_app.h"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/vec_swizzle.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/random.hpp>
#include <glm/gtx/component_wise.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/transform.hpp>

// TODO: Adding RT support for LibLava (unofficial and official)
// C++ sideway (static data only)
// C++ and C code can be mixed!
namespace vrc {
    inline static vlj::Driver driver = {};
    inline static uint32_t instanceCount = 0u;
    inline static uint32_t materialCount = 0u;

    // geometries
    inline static vlj::Framebuffer framebuffer = {};
    inline static vlj::PipelineLayout layout = {};
    inline static vlj::Background background = {};
    inline static vlj::Constants constants = {};
    inline static vlj::Rasterization rasterization = {};
    inline static vlj::RayTracing rayTracing = {};

    //
    inline static vlj::VertexSet vertexSet = {};
    inline static vlj::BufferViewSet bufferViews = {};
    inline static vlj::AttributeSet attributeSet = {};
    inline static vlj::BindingSet bindingSet = {};
    inline static std::vector<vlj::Geometry> geometries = {};

    // bodies
    inline static std::vector<vlj::GeometrySet> geometrySets = {};
    inline static std::vector<vlj::Acceleration> accelerations = {};

    // draw calls
    inline static vlj::InstanceSet instanceSet = {};
    inline static vlj::Acceleration accelerationTop = {};

    // materials
    inline static vlj::MaterialSet materialSet = {};
    inline static vlj::TextureSet textureSet = {};
    inline static vlj::SamplerSet samplerSet = {};

    // low-level data
    inline static std::vector<vkt::ImageRegion> images = {};
    inline static std::vector<vlj::SetBase> buffers = {};

    // 
    class Slots { public: 
        uint32_t count = 0u;
        std::vector<int32_t> available = {};

        // 
        Slots() {};
        Slots(const Slots& slots) : count(slots.count), available(slots.available) {};
        Slots(const uint32_t& maxCount) {
            available.clear();
            available.resize(0u);
            for (uint32_t i = 0; i < maxCount; i++) {
                available.push_back(i);
            };
        };

        // 
        Slots& operator=(const Slots& slots) {
            this->count = slots.count, this->available = slots.available;
        };

        // 
        int32_t consume() {
            int32_t id = available[count];
            if (count < available.size()) {
                int32_t& idr = available[count++]; idr = -1;
            };
            return id;
        };

        // 
        void rise(const int32_t& used) {
            if (count > 0 && used != -1 && available[count-1] == -1) {
                available[--count] = used; 
            };
        };
    };


    // data slots
    inline static Slots availableTextures = {};
    inline static Slots availableSamplers = {};
    inline static Slots availableMaterials = {};

    // geometry slots
    inline static Slots availableGeometries = {};
    inline static Slots availableGeometrySets = {};

    // per every geometry
    inline static const uint32_t accessorCount = 3, bindingsCount = 1;
    inline static const uint32_t maxInstances = 256;
    inline static const uint32_t maxGeometries = 256;


    // formats
    enum Format : uint32_t {
        FLOAT = 0,
        HALF = 1,
        INT = 2
    };

    // indices
    enum Index : uint32_t {
        NONE = 0,
        UINT8 = 1,
        UINT16 = 2,
        UINT32 = 3
    };

    // attributes
    struct AttribLayout {
        uint32_t offset = 0u;
        uint32_t count = 0u;
        Format format = Format::FLOAT;
    };

    // defaultly Neverball layout
    struct LayoutPreset {
        AttribLayout vertex = { .offset = 0u, .count = 3u, .format = Format::FLOAT };
        AttribLayout normal = { .offset = 12u, .count = 3u, .format = Format::FLOAT };
        AttribLayout texcoord = { .offset = 24u, .count = 3u, .format = Format::FLOAT };
        AttribLayout color = { .offset = 24u, .count = 0u, .format = Format::FLOAT };
    };

    // 
    class State { public:
        inline static uint32_t stride = 1u;
        inline static Index indexType = Index::NONE;
        inline static LayoutPreset preset = LayoutPreset{};
    };

    //
    void initVertexLayout(uint32_t stride, Index indexType, LayoutPreset preset) {
        State::preset = preset;
        State::stride = stride;
        State::indexType = indexType;
    };

    // 
    void encodeFormat(VkVertexInputAttributeDescription& desc, const AttribLayout& layout) {
        if (layout.format == Format::FLOAT) {
            if (layout.count == 4) { desc.format = VK_FORMAT_R32G32B32A32_SFLOAT; };
            if (layout.count == 3) { desc.format = VK_FORMAT_R32G32B32_SFLOAT; };
            if (layout.count == 2) { desc.format = VK_FORMAT_R32G32_SFLOAT; };
            if (layout.count == 1) { desc.format = VK_FORMAT_R32_SFLOAT; };
        };
        if (layout.format == Format::INT) {
            if (layout.count == 4) { desc.format = VK_FORMAT_R32G32B32A32_SINT; };
            if (layout.count == 3) { desc.format = VK_FORMAT_R32G32B32_SINT; };
            if (layout.count == 2) { desc.format = VK_FORMAT_R32G32_SINT; };
            if (layout.count == 1) { desc.format = VK_FORMAT_R32_SINT; };
        };
    };

    //
    int32_t createGeometrySet(std::vector<int32_t> geom = {}) {
        int32_t ptr = availableGeometrySets.consume();
        if (ptr >= geometrySets.size()) {
            geometrySets.resize(ptr + 1);
        };

        if (ptr >= 0) {
            std::vector<int64_t> primitiveCount = {};
            for (auto& gi : geom) {
                primitiveCount.push_back(geometries[gi].getDesc()->primitiveCount);
            };

            // 
            auto geometrySet = std::make_shared<vlr::GeometrySet>(vertexSet, vlr::DataSetCreateInfo{ .count = 1u });
            auto acceleration = std::make_shared<vlr::Acceleration>(driver, vlr::AccelerationCreateInfo{ .geometrySet = geometrySet, .initials = primitiveCount }); // Unknown Behaviour

            // 
            for (auto& gi : geom) {
                geometrySet->pushGeometry(geometries[gi]);
            };

            //
            geometrySets[ptr] = geometrySet;
            accelerations[ptr] = acceleration;
        };

        return ptr;
    };

    //
    int32_t createGeometry(vlj::SetBase vertexData, vlj::SetBase indexData, vlr::GeometryDesc desc) {
        int32_t ptr = availableGeometries.consume();
        if (ptr >= geometries.size()) {
            geometries.resize(ptr + 1);
        };

        if (ptr >= 0) {
            // 
            bufferViews.get(ptr * 2 + 0) = vertexData.getGpuBuffer();
            bufferViews.get(ptr * 2 + 1) = indexData.getGpuBuffer();

            // Bindings
            *bindingSet.get(ptr) = vkh::VkVertexInputBindingDescription{ .binding = ptr * 2 + 0, .stride = State::stride };
            if (State::indexType != Index::NONE) { desc.indexBufferView = ptr * 2 + 1; };
            if (State::indexType == Index::NONE) { desc.indexType = VK_INDEX_TYPE_NONE_KHR; };
            if (State::indexType == Index::UINT8) { desc.indexType = VK_INDEX_TYPE_UINT8_EXT; };
            if (State::indexType == Index::UINT16) { desc.indexType = VK_INDEX_TYPE_UINT16; };
            if (State::indexType == Index::UINT32) { desc.indexType = VK_INDEX_TYPE_UINT32; };

            // 
            {   // Vertex 
                const uint32_t vidx = ptr * 4 + 0;
                *attributeSet.get(vidx) = vkh::VkVertexInputAttributeDescription{ .location = 0, .binding = ptr, .offset = State::preset.vertex.offset, };
                encodeFormat(*attributeSet.get(vidx), State::preset.vertex);
                desc.vertexAttribute = vidx;
            };

            {   // Texcoord
                const uint32_t vidx = ptr * 4 + 1;
                *attributeSet.get(vidx) = vkh::VkVertexInputAttributeDescription{ .location = 1, .binding = ptr, .offset = State::preset.texcoord.offset, };
                encodeFormat(*attributeSet.get(vidx), State::preset.texcoord);
                if (State::preset.texcoord.count != 0) { desc.mesh_flags.hasTexcoord = 1; };
                desc.attributes[0] = vidx;
            };

            {   // Normals
                const uint32_t vidx = ptr * 4 + 2;
                *attributeSet.get(vidx) = vkh::VkVertexInputAttributeDescription{ .location = 2, .binding = ptr, .offset = State::preset.normal.offset, };
                encodeFormat(*attributeSet.get(vidx), State::preset.normal);
                if (State::preset.normal.count != 0) { desc.mesh_flags.hasNormal = 1; };
                desc.attributes[1] = vidx;
            };

            // 
            geometries[ptr] = std::make_shared<vlr::Geometry>(vertexSet, desc);
        };

        return ptr;
    };

    // 
    void initFramebuffer(const uint32_t& width, const uint32_t& height) {
        framebuffer.createFramebuffer(width, height);
        layout->setFramebuffer(framebuffer);
    };

    // 
    void initialize(const uint32_t& deviceID) {
        driver = std::make_shared<vlr::Driver>();
        auto instance = driver->createInstance();
        auto physicalDevice = driver->getPhysicalDevice(0u);
        auto device = driver->createDevice(true, "./", false);
        auto queue = driver->getQueue();
        auto commandPool = driver->getCommandPool();
        auto allocator = driver->getAllocator();

        // data slots
        availableTextures = Slots(256);
        availableSamplers = Slots(256);
        availableMaterials = Slots(256);

        // geometry slots
        availableGeometries = Slots(maxGeometries);
        availableGeometrySets = Slots(256);

        // 
        constants = std::make_shared<vlr::Constants>(driver, vlr::DataSetCreateInfo{ .count = 1u, .uniform = true });
        layout = std::make_shared<vlr::PipelineLayout>(driver);
        materialSet = std::make_shared<vlr::MaterialSet>(driver, vlr::DataSetCreateInfo{ .count = 256 });
        textureSet = std::make_shared<vlr::TextureSet>(driver);
        samplerSet = std::make_shared<vlr::SamplerSet>(driver);
        background = std::make_shared<vlr::Background>(driver);

        // 
        bindingSet = std::make_shared<vlr::BindingSet>(driver, vlr::DataSetCreateInfo{ .count = maxGeometries });
        attributeSet = std::make_shared<vlr::AttributeSet>(driver, vlr::DataSetCreateInfo{ .count = maxGeometries * 4u });
        bufferViews = std::make_shared<vlr::BufferViewSet>(driver);
        vertexSet = std::make_shared<vlr::VertexSet>(driver, vlr::VertexSetCreateInfo{
            .bindings = bindingSet,
            .attributes = attributeSet,
            .bufferViews = bufferViews
        });

        // 
        rasterization = std::make_shared<vlr::Rasterization>(driver, vlr::PipelineCreateInfo{
            .layout = layout,
            .framebuffer = framebuffer,
            .instanceSet = instanceSet,
            .constants = constants
        });

        // 
        rayTracing = std::make_shared<vlr::RayTracing>(driver, vlr::RayTracingCreateInfo{
            .layout = layout,
            .framebuffer = framebuffer,
            .accelerationTop = accelerationTop,
            .constants = constants
        });

        // make instanceSet
        instanceSet = std::make_shared<vlr::InstanceSet>(driver, vlr::DataSetCreateInfo{ .count = maxInstances });
        accelerationTop = std::make_shared<vlr::Acceleration>(driver, vlr::AccelerationCreateInfo{ .instanceSet = instanceSet, .initials = { maxInstances } }); // shapes.size()

        // 
        framebuffer = std::make_shared<vlr::Framebuffer>(driver);
        framebuffer.createRenderPass();

        // 
        layout->setBackground(background);
        layout->setMaterials(materialSet, textureSet, samplerSet);
        layout->setVertexSet(vertexSet);

        // 
        rasterization->setDescriptorSets(layout);
        rayTracing->setDescriptorSets(layout);
    };
};

// implementation for C API (bridge between C++ and C)
extern "C" {
    #include "./renderer.h"

    // initialize defaults for C
#ifdef VLR_C_RENDERER
    void initMaterialUnit(CMaterialUnit* cunit) {
        const auto unit = vlr::MaterialUnit{};
        *cunit = reinterpret_cast<const CMaterialUnit&>(unit);
    };
#endif
};

// implementation for C++ wrapper API (bridge between hardcode and softcode)
#include "./renderer.hpp"
namespace vrp {
#ifdef VLR_CPP_RENDERER

#endif
};
