#pragma once
#include "./Config.hpp"
#include "./Driver.hpp"
#include "./PipelineLayout.hpp"

#ifdef ENABLE_OPTIX_DENOISE
#include "./OptiXDenoise.hpp"
#endif

namespace vlr {

    struct PipelineCreateInfo {
        vkt::uni_ptr<PipelineLayout> layout = {};
        vkt::uni_ptr<Framebuffer> framebuffer = {};
        vkt::uni_ptr<InstanceSet> instanceSet = {};
        vkt::uni_ptr<Constants> constants = {};
        std::string vertexShader = "./shaders/rasterize.vert.spv";
        std::string geometryShader = "./shaders/rasterize.geom.spv";
        std::string fragmentShader = "./shaders/rasterize.frag.spv";
    };

    struct RayTracingCreateInfo {
        vkt::uni_ptr<PipelineLayout> layout = {};
        vkt::uni_ptr<Framebuffer> framebuffer = {};
        vkt::uni_ptr<Acceleration> accelerationTop = {}; // Top Level for Ray Tracing
        vkt::uni_ptr<Constants> constants = {};
        std::string generationShader = "./shaders/generation.comp.spv";
        std::string intersectionShader = "./shaders/intersection.comp.spv";
        std::string interpolationShader = "./shaders/interpolation.comp.spv";
        std::string resampleShader = "./shaders/resample.comp.spv";
        std::string finalizeShader = "./shaders/finalize.comp.spv";
        std::string compositeShader = "./shaders/composite.comp.spv";
#ifdef ENABLE_OPTIX_DENOISE
        vkt::uni_ptr<OptiXDenoise> denoise = {};
#endif
    };

    struct RenderPass {
        VkFramebuffer framebuffer = {};
        VkRenderPass renderPass = {};
        
        std::vector<vkh::VkPipelineColorBlendAttachmentState> blendStates = {};
        std::vector<vkh::VkClearValue> clearValues = {};
        vkh::VsDescriptorSetCreateInfoHelper descriptorSetInfo = {};
    };

    class Framebuffer : public std::enable_shared_from_this<Framebuffer> { protected: friend Rasterization; friend Resampling; friend RayTracing; friend PipelineLayout;
        RenderPass rasterFBO = {}, resampleFBO = {}; VkDescriptorSet set = {};
        vkt::ImageRegion depthStencilImage = {}, atomicMapping = {};
        std::vector<VkSampler> samplers = {};
        std::vector<vkt::ImageRegion> currentsImages = {};  // Current Frame
        std::vector<vkt::ImageRegion> previousImages = {};  // Previous Frame
        std::vector<vkt::ImageRegion> resampleImages = {}; // Resampled Previous Frame
        std::vector<vkt::ImageRegion> rasterImages = {};    // Rasterized Frame
        VkRect2D scissor = {}; VkViewport viewport = {};    // 
        uint32_t width = 1920u, height = 1200u;
        vkt::uni_ptr<Driver> driver = {};
        bool updated = false;

    public: 
        Framebuffer() { this->constructor(); };
        Framebuffer(vkt::uni_ptr<Driver> driver) { this->constructor(driver); };
        ~Framebuffer() {};

        // 
        virtual void constructor() {};
        virtual void constructor(vkt::uni_ptr<Driver> driver);
        virtual void createRenderPass();
        virtual void createFramebuffer(uint32_t width = 1920u, uint32_t height = 1200u);
        virtual void createDescriptorSet(vkt::uni_ptr<PipelineLayout> layout);
    };

};
